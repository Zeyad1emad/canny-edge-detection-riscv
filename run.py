import argparse
import subprocess
import cv2
import numpy as np
import os
import sys
import re

def extract_cycles(log_text):
    """دالة لاستخراج عدد الـ Clock Cycles من نصوص الـ C++"""
    match = re.search(r'AVERAGE CYCLES\s*:\s*(\d+)', log_text)
    return int(match.group(1)) if match else None

def main():
    # 1. إعداد الـ Arguments
    parser = argparse.ArgumentParser(description="Run RISC-V Canny Edge Detection Pipeline")
    # ضفنا اختيار 'benchmark' للمود
    parser.add_argument("-m", "--mode", choices=["scalar", "rvv", "benchmark"], required=True, help="Execution mode (scalar, rvv, or benchmark)")
    parser.add_argument("-i", "--input", required=True, help="Input image path (e.g., car.jpg)")
    # خلينا الـ output مش إجباري عشان لو بنعمل benchmark بس
    parser.add_argument("-o", "--output", default="output.png", help="Output image path (e.g., output.png)")
    parser.add_argument("--low", type=int, default=50, help="Low threshold (default: 50)")
    parser.add_argument("--high", type=int, default=150, help="High threshold (default: 150)")

    args = parser.parse_args()

    print(f"[*] Mode: {args.mode.upper()}")
    print(f"[*] Input Image: {args.input}")

    # 2. قراءة الصورة وتجهيز الأبعاد
    img = cv2.imread(args.input)
    if img is None:
        print(f"[!] Error: Could not read image at '{args.input}'. Check the path.")
        sys.exit(1)

    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    h, w = gray.shape
    print(f"[*] Dimensions: {w}x{h}")

    temp_in = "temp_in.raw"
    temp_out = "temp_out.raw"
    gray.astype(np.uint8).tofile(temp_in)

    # دالة مساعدة لتشغيل QEMU وتجميع اللوجات
    def run_simulation(binary_path):
        if not os.path.exists(binary_path):
            print(f"[!] Error: {binary_path} not found! Compile first.")
            sys.exit(1)
            
        cmd = ["qemu-riscv64", binary_path, str(w), str(h), str(args.low), str(args.high)]
        try:
            with open(temp_in, "rb") as f_in, open(temp_out, "wb") as f_out:
                # بنستقبل الـ Logs من stderr عشان نحللها بعدين
                res = subprocess.run(cmd, stdin=f_in, stdout=f_out, stderr=subprocess.PIPE, text=True, check=True)
                return res.stderr
        except subprocess.CalledProcessError as e:
            print(f"[!] Error during execution of {binary_path}: {e}")
            sys.exit(1)

    # 3. منطق التشغيل العادي (استخراج صورة)
    if args.mode in ["scalar", "rvv"]:
        binary = "./pipeline_rv_scalar.out" if args.mode == "scalar" else "./pipeline_rvv.out"
        print(f"[*] Executing Pipeline...")
        
        # تشغيل المحاكي وطباعة التقرير اللي طالع من الـ C++
        log_output = run_simulation(binary)
        print(log_output)
        
        # حفظ الصورة
        out_data = np.fromfile(temp_out, dtype=np.uint8)
        expected_size = w * h
        if len(out_data) < expected_size:
            print(f"[!] Error: Output size mismatch.")
            sys.exit(1)
            
        out_img = out_data[:expected_size].reshape((h, w))
        cv2.imwrite(args.output, out_img)
        print(f"[+] Success! Edge detection saved to: {args.output}")

    # 4. منطق حساب معامل التسريع (Benchmark)
    elif args.mode == "benchmark":
        print("\n[*] Running SCALAR mode for baseline...")
        scalar_log = run_simulation("./pipeline_rv_scalar.out")
        scalar_cycles = extract_cycles(scalar_log)
        
        print("[*] Running RVV mode for speedup calculation...")
        rvv_log = run_simulation("./pipeline_rvv.out")
        rvv_cycles = extract_cycles(rvv_log)
        
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
        else:
            print("[!] Error: Could not extract cycles from output logs.")

    # 5. تنظيف الملفات المؤقتة
    if os.path.exists(temp_in): os.remove(temp_in)
    if os.path.exists(temp_out): os.remove(temp_out)

if __name__ == "__main__":
    main()