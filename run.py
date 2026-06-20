import argparse
import subprocess
import cv2
import numpy as np
import os
import sys
import re

def extract_cycles(log_text):
    """Extract the number of Cycles from the output"""
    match = re.search(r"AVERAGE CYCLES\s*:\s*(\d+)", log_text)
    if match:
        return int(match.group(1))
    return None

def run_pipeline(binary, w, h, low, high, temp_in, temp_out):
    """Helper function to run the emulator and capture reports from stderr"""
    cmd = ["qemu-riscv64", binary, str(w), str(h), str(low), str(high)]
    try:
        with open(temp_in, "rb") as f_in, open(temp_out, "wb") as f_out:
            # Redirect stdin for Input and stdout for Output
            # Redirect stderr to PIPE to read performance reports and cycles
            result = subprocess.run(cmd, stdin=f_in, stdout=f_out, stderr=subprocess.PIPE, check=True)
            return result.stderr.decode('utf-8', errors='ignore')
    except subprocess.CalledProcessError as e:
        print(f"[!] Error during QEMU execution: {e}")
        if os.path.exists(temp_in): os.remove(temp_in)
        sys.exit(1)

def main():
    # 1. Setup Arguments (Flags)
    parser = argparse.ArgumentParser(description="Run RISC-V Canny Edge Detection Pipeline")
    
    # Added benchmark to choices
    parser.add_argument("-m", "--mode", choices=["scalar", "rvv", "benchmark"], required=True, help="Execution mode (scalar, rvv, or benchmark)")
    parser.add_argument("-i", "--input", required=True, help="Input image path (e.g., car.jpg)")
    
    # Removed required=True to facilitate benchmark mode
    parser.add_argument("-o", "--output", default="output.png", help="Output image path (e.g., output.png)")
    parser.add_argument("--low", type=int, default=50, help="Low threshold (default: 50)")
    parser.add_argument("--high", type=int, default=150, help="High threshold (default: 150)")

    args = parser.parse_args()

    print(f"[*] Mode: {args.mode.upper()}")
    print(f"[*] Input Image: {args.input}")

    # 2. Read image and setup dimensions
    img = cv2.imread(args.input)
    if img is None:
        print(f"[!] Error: Could not read image at '{args.input}'. Check the path.")
        sys.exit(1)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    h, w = gray.shape
    print(f"[*] Dimensions: {w}x{h}")

    temp_in = "temp_in.raw"
    temp_out = "temp_out.raw"

    # Convert image to raw bytes
    gray.astype(np.uint8).tofile(temp_in)

    # =========================================================
    # Normal Execution Mode (Scalar or RVV)
    # =========================================================
    if args.mode in ["scalar", "rvv"]:
        binary = "./pipeline_rv_scalar.out" if args.mode == "scalar" else "./pipeline_rvv.out"

        if not os.path.exists(binary):
            print(f"[!] Error: {binary} not found! Compile the project first.")
            sys.exit(1)

        binary_size = os.path.getsize(binary)
        print(f"[*] Binary Size: {binary_size:,} bytes")
        
        print(f"[*] Executing: {' '.join(['qemu-riscv64', binary, str(w), str(h), str(args.low), str(args.high)])} < {temp_in} > {temp_out}")
        
        # Execute and print report (Logs)
        log_output = run_pipeline(binary, w, h, args.low, args.high, temp_in, temp_out)
        print("\n" + log_output) 

        # 6. Read output and convert to image
        if not os.path.exists(temp_out) or os.path.getsize(temp_out) == 0:
            print("[!] Error: Output raw file is empty or missing.")
            sys.exit(1)

        out_data = np.fromfile(temp_out, dtype=np.uint8)
        expected_size = w * h

        if len(out_data) < expected_size:
            print(f"[!] Error: Output size mismatch. Expected {expected_size}, got {len(out_data)}")
            sys.exit(1)

        # Convert raw data to pixel array and save
        out_img = out_data[:expected_size].reshape((h, w))
        cv2.imwrite(args.output, out_img)
        print(f"[+] Success! Edge detection saved to: {args.output}")

    # =========================================================
    # Benchmark Mode to calculate Speedup
    # =========================================================
    elif args.mode == "benchmark":
        scalar_bin = "./pipeline_rv_scalar.out"
        rvv_bin = "./pipeline_rvv.out"

        if not os.path.exists(scalar_bin) or not os.path.exists(rvv_bin):
            print("[!] Error: Both executables must be compiled for benchmark mode.")
            sys.exit(1)

        print(f"\n[*] Running SCALAR mode for baseline... (Binary: {os.path.getsize(scalar_bin):,} bytes)")
        scalar_log = run_pipeline(scalar_bin, w, h, args.low, args.high, temp_in, temp_out)
        scalar_cycles = extract_cycles(scalar_log)
        if scalar_cycles:
            print(f"    -> Scalar Cycles: {scalar_cycles:,}")
        else:
            print("    [!] Could not extract scalar cycles.")

        print(f"\n[*] Running RVV mode for speedup calculation... (Binary: {os.path.getsize(rvv_bin):,} bytes)")
        rvv_log = run_pipeline(rvv_bin, w, h, args.low, args.high, temp_in, temp_out)
        rvv_cycles = extract_cycles(rvv_log)
        if rvv_cycles:
            print(f"    -> RVV Cycles   : {rvv_cycles:,}")
        else:
            print("    [!] Could not extract RVV cycles.")

        if scalar_cycles and rvv_cycles:
            speedup = scalar_cycles / rvv_cycles
            print("\n" + "="*45)
            print(" 🚀 SPEEDUP CALCULATION REPORT")
            print("="*45)
            print(f" Image            : {args.input} ({w}x{h})")
            print(f" Scalar Cycles    : {scalar_cycles:,}")
            print(f" RVV Cycles       : {rvv_cycles:,}")
            print("-" * 45)
            print(f" Speedup Ratio    : {speedup:.3f}x")
            print("="*45)

    # 7. Clean up temporary files
    if os.path.exists(temp_in): os.remove(temp_in)
    if os.path.exists(temp_out): os.remove(temp_out)

if __name__ == "__main__":
    main()