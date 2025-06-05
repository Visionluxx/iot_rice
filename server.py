from flask import Flask, request, jsonify, render_template
from datetime import datetime

app = Flask(__name__)

# Lưu dữ liệu vào bộ nhớ tạm (RAM), bạn có thể thay bằng database
data_log = []

@app.route('/')
def index():
    return render_template('index.html', data=data_log)

@app.route('/api/data', methods=['POST'])
def receive_data():
    try:
        content = request.json
        record = {
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "water_level_cm": content.get("water_level_cm"),
            "tds_ppm": content.get("tds_ppm")
        }
        data_log.append(record)
        print("Received:", record)
        return jsonify({"status": "success", "received": record}), 200
    except Exception as e:
        print("Error:", e)
        return jsonify({"status": "error", "message": str(e)}), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
