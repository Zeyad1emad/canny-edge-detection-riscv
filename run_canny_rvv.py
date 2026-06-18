import sys
import os
import cv2
import numpy as np

# Path to the cross-compiled RISC-V Vector executable (from Makefile)
CPP_BINARY = "./pipeline_rvv.out"

def process_image(input_path, output_path, low_thresh=50, high_thresh=150):
    print(f"[*] Reading input image: {input_path}")
    src = cv2.imread(input_path)
    if src is None:
        print(f"Error: Could not read image at {input_path}")
        sys.exit(1)

    # Convert to grayscale and extract dimensions
    gray = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY)
    height, width = gray.shape
    print(f"[*] Grayscale image dimensions: {width}x{height}")

    # FIXED: Using absolute path in system /tmp directory to avoid permission or path issues with QEMU
    temp_in = "/tmp/input_temp.raw"
    temp_out = "/tmp/output_temp.raw"
    
    # Save grayscale pixels as raw bytes
    gray.astype(np.uint8).tofile(temp_in)

    # Build and execute the RISC-V command via QEMU emulator
    cmd = f"qemu-riscv64 {CPP_BINARY} {width} {height} {temp_in} {temp_out} {low_thresh} {high_thresh}"
    print(f"[*] Executing RVV Canny Pipeline via QEMU: {cmd}")
    
    if os.system(cmd) != 0:
        print("Error: RVV application execution via QEMU failed!")
        if os.path.exists(temp_in): os.remove(temp_in)
        sys.exit(1)

    # Read the generated raw edges back from disk
    if not os.path.exists(temp_out):
        print(f"Error: RVV app did not generate {temp_out}")
        if os.path.exists(temp_in): os.remove(temp_in)
        sys.exit(1)
        
    raw_data = np.fromfile(temp_out, dtype=np.uint8)
    edge_image = raw_data[:width * height].reshape((height, width))
    
    # Save final edge map as a standard image file (PNG/JPG)
    cv2.imwrite(output_path, edge_image)

    # Cleanup temporary pipeline assets
    if os.path.exists(temp_in): os.remove(temp_in)
    if os.path.exists(temp_out): os.remove(temp_out)
    print(f"[+] Success! RVV Edge detection complete. Result saved to: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 run_canny_rvv.py <input_image.jpg/png> <output_image.png> [low_thresh] [high_thresh]")
        sys.exit(1)
    
    low = sys.argv[3] if len(sys.argv) > 3 else 50
    high = sys.argv[4] if len(sys.argv) > 4 else 150
    process_image(sys.argv[1], sys.argv[2], low, high)