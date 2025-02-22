from flask import Flask, request, jsonify, render_template
from flask_sqlalchemy import SQLAlchemy
from flask_cors import CORS
from datetime import datetime
import pymysql  # Required for MySQL connections

app = Flask(__name__)
CORS(app)

# 🔹 REPLACE with your actual database connection details
DATABASE_URI = "mysql+pymysql://root:aadil7106@localhost/saffron"
app.config['SQLALCHEMY_DATABASE_URI'] = DATABASE_URI
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

# 🔹 Match this model with your existing SQL table
class SensorData(db.Model):
    __tablename__ = "sensor_data"  # 🔹 Use your actual table name  # 🔹 Ensure this matches your table's primary key
    Temperature = db.Column(db.Float, nullable=False)
    Humidity = db.Column(db.Float, nullable=False)
    Soil_moisture = db.Column(db.Integer, nullable=False)
    Water_level = db.Column(db.Integer, nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)

# 🔹 Check if Flask successfully connects to your database
with app.app_context():
    try:
        db.session.execute("SELECT 1")  # Test database connection
        print("✅ Connected to database successfully!")
    except Exception as e:
        print(f"❌ Database connection failed: {e}")

# 🔹 Route to receive data from ESP32 and store it in the existing table
@app.route('/temperature', methods=['POST'])
def receive_data():
    data = request.json

    if not data:
        return jsonify({"message": "No data received"}), 400

    Temperature = data.get('Temperature')
    Humidity = data.get('Humidity')
    Soil_moisture = data.get('Soil_moisture')
    Water_level = data.get('Water_level')

    if None in [Temperature, Humidity, Soil_moisture, Water_level]:
        return jsonify({"message": "Invalid data"}), 400

    new_entry = SensorData(
        Temperature=Temperature,
        Humidity=Humidity,
        Soil_moisture=Soil_moisture,
        Water_level=Water_level
    )

    db.session.add(new_entry)
    db.session.commit()

    return jsonify({"message": "Data inserted successfully"}), 201


# 🔹 Route to display latest data on a webpage
@app.route('/humidity',methods=['GET'])
def get_latest_data():
    latest_entry = SensorData.query.order_by(SensorData.id.desc()).first()

    if not latest_entry:
        return jsonify({"message": "No data available"}), 404

    return jsonify({
        "temperature": latest_entry.Temperature,
        "humidity": latest_entry.Humidity,
        "soil_moisture": latest_entry.Soil_moisture,
        "water_level": latest_entry.Water_level,
        "timestamp": latest_entry.timestamp.strftime('%Y-%m-%d %H:%M:%S')
    })
def display_data():
    latest_entry = SensorData.query.order_by(SensorData.id.desc()).first()
    return render_template('index.html', data=latest_entry)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
