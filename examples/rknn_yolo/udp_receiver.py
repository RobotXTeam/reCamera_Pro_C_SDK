import socket
import struct
import cv2
import numpy as np

def main():
    port = int(__import__("os").environ.get("RECAMERA_UDP_PORT", "9090"))
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", port))
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
    
    print(f"正在监听 UDP {port} 端口接收 MJPEG 分片流...")
    
    current_frame_id = None
    current_frame = None
    current_total = 0
    received_chunks = set()
    
    cv2.namedWindow("reCamera MJPEG UDP Stream", cv2.WINDOW_NORMAL)
    
    while True:
        data, addr = sock.recvfrom(65535)
        if len(data) < 32:
            continue
        
        magic = data[0:4]
        if magic != b'MJPG': continue

        version, header_size = struct.unpack_from('<HH', data, 4)
        frame_id, total_size, offset = struct.unpack_from('<III', data, 8)
        chunk_size, chunk_index, chunk_count = struct.unpack_from('<HHH', data, 20)
        width, height, flags = struct.unpack_from('<HHH', data, 26)
        
        osd_text = ""
        if header_size >= 32:
            osd_text_bytes = data[32:header_size]
            osd_text = osd_text_bytes.decode('utf-8', 'ignore').rstrip('\x00')

        if version != 2 or header_size > len(data):
            continue
        payload = data[header_size:]
        if len(payload) != chunk_size or offset + chunk_size > total_size:
            continue

        if current_frame_id != frame_id:
            current_frame_id = frame_id
            current_frame = bytearray(total_size)
            current_total = total_size
            received_chunks = set()

        if current_frame is None:
            continue

        current_frame[offset:offset + chunk_size] = payload
        received_chunks.add(chunk_index)

        if len(received_chunks) == chunk_count:
            img = cv2.imdecode(np.frombuffer(current_frame, dtype=np.uint8), cv2.IMREAD_COLOR)
            if img is not None:
                if osd_text:
                    parts = osd_text.split(';')
                    fps_text = parts[0]
                    # Draw FPS text background for better visibility
                    (fw, fh), _ = cv2.getTextSize(fps_text, cv2.FONT_HERSHEY_SIMPLEX, 1.0, 2)
                    cv2.rectangle(img, (15, 10), (25 + fw, 50), (0, 0, 0), -1)
                    cv2.putText(img, fps_text, (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 0), 2, cv2.LINE_AA)
                    
                    # COCO 80 classes
                    coco_classes = [
                        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
                        "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
                        "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
                        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
                        "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
                        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
                        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
                        "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
                        "hair drier", "toothbrush"
                    ]
                    
                    colors = [(0, 165, 255), (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 0, 255), (255, 255, 0)]
                    for bbox_str in parts[1:]:
                        if not bbox_str: continue
                        b_parts = bbox_str.split(',')
                        if len(b_parts) == 6:
                            try:
                                x1, y1, x2, y2 = map(int, b_parts[:4])
                                cls_id = int(b_parts[4])
                                conf = float(b_parts[5])
                                color = colors[cls_id % len(colors)]
                                class_name = coco_classes[cls_id] if 0 <= cls_id < len(coco_classes) else f"cls{cls_id}"
                                label = f"{class_name} {conf:.2f}"
                                
                                # Ultralytics style bounding box
                                cv2.rectangle(img, (x1, y1), (x2, y2), color, 3)
                                
                                # Label background
                                (w, h), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.7, 2)
                                label_y1 = max(0, y1 - h - 10)
                                cv2.rectangle(img, (x1, label_y1), (x1 + w, label_y1 + h + 10), color, -1)
                                cv2.putText(img, label, (x1, label_y1 + h + 5), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2, cv2.LINE_AA)
                            except ValueError:
                                pass
                cv2.imshow("reCamera MJPEG UDP Stream", img)
                if cv2.waitKey(1) == 27:
                    break
            current_frame_id = None
            current_frame = None
            current_total = 0
            received_chunks = set()

    cv2.destroyAllWindows()
    sock.close()

if __name__ == '__main__':
    main()
