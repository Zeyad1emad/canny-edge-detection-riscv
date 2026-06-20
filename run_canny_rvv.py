import sys
import os
import cv2
import numpy as np

def process_image(input_path, output_path, binary_path, low_thresh=50, high_thresh=150):
    print(f"[*] Reading input image: {input_path}")
    src = cv2.imread(input_path)
    if src is None:
        print(f"Error: Could not read image at {input_path}")
        sys.exit(1)

    gray = cv2.cvtColor(src, cv2.COLOR_BGR2GRAY)
    height, width = gray.shape
    print(f"[*] Grayscale image dimensions: {width}x{height}")

    temp_in = "input_temp.raw"
    temp_out = "output_temp.raw"
    
    gray.astype(np.uint8).tofile(temp_in)

    cmd = f"qemu-riscv64 {binary_path} {width} {height} {temp_in} {temp_out} {low_thresh} {high_thresh}"
    print(f"[*] Executing Pipeline:\n    {cmd}")
    
    ret = os.system(cmd)
    if ret != 0:
        print(f"Error: Application execution failed! (Binary: {binary_path})")
        if os.path.exists(temp_in): os.remove(temp_in)
        sys.exit(1)

    raw_data = np.fromfile(temp_out, dtype=np.uint8)
    expected_size = width * height
    
    if len(raw_data) < expected_size:
        print(f"Error: Output data size mismatch.")
        sys.exit(1)

    edge_image = raw_data[:expected_size].reshape((height, width))
    cv2.imwrite(output_path, edge_image)

    if os.path.exists(temp_in): os.remove(temp_in)
    if os.path.exists(temp_out): os.remove(temp_out)
    print(f"[+] Success! Edge detection complete. Result saved to: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 run_canny.py <mode: rvv|scalar> <input_image> <output_image>")
        sys.exit(1)
    
    mode = sys.argv[1].lower()
    input_img = sys.argv[2]
    output_img = sys.argv[3]
    
    if mode == "rvv":
        CPP_BINARY = "./pipeline_rvv.out"
    elif mode == "scalar":
        CPP_BINARY = "./pipeline_rv_scalar.out"
    else:
        print("Error: Invalid mode. Use 'rvv' or 'scalar'.")
        sys.exit(1)

    low = int(sys.argv[4]) if len(sys.argv) > 4 else 50
    high = int(sys.argv[5]) if len(sys.argv) > 5 else 150
    
    print(f"\n--- Running in {mode.upper()} mode ---")
    process_image(input_img, output_img, CPP_BINARY, low, high)