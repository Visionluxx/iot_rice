from flask import Flask, request, jsonify, render_template
from datetime import datetime

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

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
