import argparse
import sys
import time
import cv2
import serial
from tflite_support.task import core
from tflite_support.task import processor
from tflite_support.task import vision
import utils
from picamera2 import Picamera2
import numpy as np


def run(model: str, width: int, height: int, num_threads: int,
        enable_edgetpu: bool, score_threshold: float) -> None:
    """Continuously run inference on images acquired from the camera.

    Args:
        model: Name of the TFLite object detection model.
        width: The width of the frame captured from the camera.
        height: The height of the frame captured from the camera.
        num_threads: The number of CPU threads to run the model.
        enable_edgetpu: True/False whether the model is a EdgeTPU model.
        score_threshold: The score threshold for detection results.
    """

    # Initialize serial communication with Arduino
    #arduino = serial.Serial('/dev/ttyACM0', 115200)  # Change '/dev/ttyACM0' to your Arduino port

    # Variables to calculate FPS
    counter, fps = 0, 0
    start_time = time.time()

    # Initialize the camera
    picam2 = Picamera2()
    camera_config = picam2.create_still_configuration(main={"size": (width, height)})
    picam2.configure(camera_config)
    picam2.start()

    # Visualization parameters
    row_size = 20  # pixels
    left_margin = 24  # pixels
    text_color = (0, 0, 255)  # red
    font_size = 1
    font_thickness = 1
    fps_avg_frame_count = 10

    # Initialize the object detection model
    base_options = core.BaseOptions(
        file_name=model, num_threads=num_threads, use_coral=enable_edgetpu)
    detection_options = processor.DetectionOptions(
        max_results=3, score_threshold=score_threshold)
    options = vision.ObjectDetectorOptions(
        base_options=base_options, detection_options=detection_options)
    detector = vision.ObjectDetector.create_from_options(options)

    # Continuously capture images from the camera and run inference
    while True:
        frame = picam2.capture_array()
        if frame is None:
            sys.exit('ERROR: Unable to read from camera. Please verify your camera settings.')

        counter += 1
        frame = cv2.flip(frame, 1)

        # Convert the image from BGR to RGB as required by the TFLite model.
        rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        # Create a TensorImage object from the RGB image.
        input_tensor = vision.TensorImage.create_from_array(rgb_image)

        # Run object detection estimation using the model.
        detection_result = detector.detect(input_tensor)

        # Draw keypoints and edges on input image and calculate centroids
        frame, centroids = utils.visualize_and_get_centroids(frame, detection_result)

        # Print and send the centroids of detected objects
        #for i, centroid in enumerate(centroids):
        #    x, y = centroid
            #print(f"Object {i+1}: Centroid at {centroid}")
         #   arduino.write(f'{x}\n'.encode())  # Send x-coordinate to Arduino
          #  time.sleep(1)

        # Convert the image back to BGR for OpenCV display
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

        # Calculate the FPS
        if counter % fps_avg_frame_count == 0:
            end_time = time.time()
            fps = fps_avg_frame_count / (end_time - start_time)
            start_time = time.time()

        # Show the FPS
        fps_text = 'FPS = {:.1f}'.format(fps)
        text_location = (left_margin, row_size)
        cv2.putText(frame, fps_text, text_location, cv2.FONT_HERSHEY_PLAIN,
                    font_size, text_color, font_thickness)

        # Stop the program if the ESC key is pressed.
        if cv2.waitKey(1) == 27:
            break
        cv2.imshow('object_detector', frame)

    picam2.close()
    #arduino.close()
    cv2.destroyAllWindows()


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--model',
        help='Path of the object detection model.',
        required=False,
        default='best.tflite')
    parser.add_argument(
        '--frameWidth',
        help='Width of frame to capture from camera.',
        required=False,
        type=int,
        default=640)
    parser.add_argument(
        '--frameHeight',
        help='Height of frame to capture from camera.',
        required=False,
        type=int,
        default=480)
    parser.add_argument(
        '--numThreads',
        help='Number of CPU threads to run the model. (Max: 4 for Raspberry Pi 4)',
        required=False,
        type=int,
        default=4)
    parser.add_argument(
        '--enableEdgeTPU',
        help='Whether to run the model on EdgeTPU.',
        action='store_true',
        required=False,
        default=False)
    parser.add_argument(
        '--scoreThreshold',
        help='The score threshold for detection results.',
        required=False,
        type=float,
        default=0.5)
    args = parser.parse_args()

    run(args.model, args.frameWidth, args.frameHeight,
        args.numThreads, args.enableEdgeTPU, args.scoreThreshold)


if __name__ == '__main__':
    main()
