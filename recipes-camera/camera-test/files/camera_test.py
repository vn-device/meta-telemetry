import subprocess
import os

def test_camera():
    # Define a writable path on the target device
    output_directory = "/home/root/camera_captures"
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)
        
    output_file = os.path.join(output_directory, "test_image.jpg")
    
    cmd = ["rpicam-still", "-o", output_file, "--width", "1920", "--height", "1080", "--timeout", "2000"]
    
    try:
        subprocess.run(cmd, check=True)
        print(f"Success! Image saved to {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error executing camera command: {e}")

if __name__ == "__main__":
    test_camera()