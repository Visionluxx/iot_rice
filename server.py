from flask import Flask, request, jsonify, render_template
from datetime import datetime

import gdown
# Download mô hình từ GG Drive
file_id = "1pVR2WDUMkXUE1E4fD6-pe4VSMcvsxAJI"
url = f"https://drive.google.com/uc?id={file_id}"
# Tên file lưu về local
output = "diseases_detection.keras"
gdown.download(url, output, quiet=False)
# Load từ loacal
from tensorflow.keras.models import load_model
model = load_model(output)

app = Flask(__name__)

# Lưu dữ liệu vào bộ nhớ tạm (RAM), có thể thay bằng database
data_log = []

@app.route('/')
def index():
    return render_template('index.html', data=data_log)

@app.route('/api/data', methods=['POST'])
def receive_data():
    try:
        content = request.json
        tds=content.get("tds_ppm")
        warning=""
        if tds<=300:
            warning="nguồn nước có độ mặn thích hợp"
        else if 300<tds and tds<500:
            warning="nguồn nước hơi mặn, hạn chế tưới tiêu"
        else if tds>500:
            warning = "nguồn nước quá mặn, không thể tưới tiêu"
        record = {
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "water_level_cm": content.get("water_level_cm"),
            "tds_ppm": content.get("tds_ppm")
            "water_tds"=warning
        }
        data_log.append(record)
        print("Received:", record)
        return jsonify({"status": "success", "received": record}), 200
    except Exception as e:
        print("Error:", e)
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/api/image', methods=['POST'])
def receive_image():
    try:
        content = request.get_json()
        image_base64 = content.get("image")
        
        # Giải mã và lưu file ảnh
        import base64
        with open("image.jpg", "wb") as f:
            f.write(base64.b64decode(image_base64))
        from PIL import Image
        import numpy as np
        from tensorflow.keras.models import load_model
        img=Image.open("image.jpg").resize((224, 224))
        img_array=np.expand_dims(np.array(img)/255.0, axis=0)
        pred=model.predict(img_array)
        class_idx=int(np.argmax(pred[0]))
        result=""
        if class_idx==3:
            result="Lúa khỏe mạnh"
        else:
            result="Lúa đã bị bệnh, hãy hành động"
        data_log.append(result)
        return jsonify({"status": "received"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 400


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
