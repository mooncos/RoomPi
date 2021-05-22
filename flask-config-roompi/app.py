from flask import Flask, request, render_template, jsonify, make_response

app = Flask(__name__)

profiles = {
    "Default" : {
        "temp_crit_low": 5.0,
        "temp_crit_high": 33.0,
        "temp_warn_low": 18.0,
        "temp_warn_high": 28.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 82.0,
        "rh_warn_low": 30.0,
        "rh_warn_high": 70.0,
        "lux_crit": 150,
        "lux_warn": 350,
        "eco2_crit": 4000,
        "eco2_warn": 2000,
        "meas_t_ms": 60000,
        "dht11_t_ms": 5000,
        "bh1750_t_ms": 5000,
        "ccs811_t_ms": 5000,
        "output_t_ms": 5000    
    },
    "Aulas B" : {
        "temp_crit_low": 10.0,
        "temp_crit_high": 35.5,
        "temp_warn_low": 20.0,
        "temp_warn_high": 30.0,
        "rh_crit_low": 5.0,
        "rh_crit_high": 90.0,
        "rh_warn_low": 20.0,
        "rh_warn_high": 80.0,
        "lux_crit": 90,
        "lux_warn": 420,
        "eco2_crit": 4000,
        "eco2_warn": 2500
    },
    "Biblioteca" : {
        "temp_crit_low": 10.0,
        "temp_crit_high": 38.5,
        "temp_warn_low": 19.0,
        "temp_warn_high": 33.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 86.5,
        "rh_warn_low": 21.0,
        "rh_warn_high": 75.0,
        "lux_crit": 0,
        "lux_warn": 400,
        "eco2_crit": 4500,
        "eco2_warn": 3000
    },
    "Hogar Urbano" : {
        "temp_crit_low": 10.0,
        "temp_crit_high": 30.0,
        "temp_warn_low": 17.0,
        "temp_warn_high": 28.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 80.0,
        "rh_warn_low": 20.0,
        "rh_warn_high": 70.0,
        "lux_crit": 80,
        "lux_warn": 250,
        "eco2_crit": 4000,
        "eco2_warn": 2000
    },
    "Gimnasio/Entrenam." : {
        "temp_crit_low": 10.0,
        "temp_crit_high": 30.0,
        "temp_warn_low": 17.0,
        "temp_warn_high": 28.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 80.0,
        "rh_warn_low": 20.0,
        "rh_warn_high": 70.0,
        "lux_crit": 80,
        "lux_warn": 250,
        "eco2_crit": 4000,
        "eco2_warn": 2000
    },
    "Quirófano/ICU/Radiolog." : {
        "temp_crit_low": 10.0,
        "temp_crit_high": 30.0,
        "temp_warn_low": 17.0,
        "temp_warn_high": 28.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 80.0,
        "rh_warn_low": 20.0,
        "rh_warn_high": 70.0,
        "lux_crit": 80,
        "lux_warn": 250,
        "eco2_crit": 4000,
        "eco2_warn": 2000
    },
    "CSIC Aulas" : {
        "temp_crit_low": 5.0,
        "temp_crit_high": 33.0,
        "temp_warn_low": 18.0,
        "temp_warn_high": 28.0,
        "rh_crit_low": 10.0,
        "rh_crit_high": 82.0,
        "rh_warn_low": 30.0,
        "rh_warn_high": 70.0,
        "lux_crit": 150,
        "lux_warn": 350,
        "eco2_crit": 2100,
        "eco2_warn": 1200
    },
    "OMS Trabajo" : {
        "temp_crit_low": 15.0,
        "temp_crit_high": 32.25,
        "temp_warn_low": 18.0,
        "temp_warn_high": 29.75,
        "rh_crit_low": 20.0,
        "rh_crit_high": 70.0,
        "rh_warn_low": 25.0,
        "rh_warn_high": 65.0,
        "lux_crit": 200,
        "lux_warn": 450,
        "eco2_crit": 2175,
        "eco2_warn": 1500
    },
    "CDC EEUU" : {
        "temp_crit_low": 22.33,
        "temp_crit_high": 35.68,
        "temp_warn_low": 15.24,
        "temp_warn_high": 31.49,
        "rh_crit_low": 20.0,
        "rh_crit_high": 70.0,
        "rh_warn_low": 30.0,
        "rh_warn_high": 60.0,
        "lux_crit": 200,
        "lux_warn": 450,
        "eco2_crit": 2680,
        "eco2_warn": 2100
    },
}

@app.route('/', methods=['GET', 'POST'])
def home():
    if request.method == "GET":
        return render_template("config.html", profiles=profiles)
    if request.method == "POST":
        print(request.form.keys())
        with open("roompi.conf", "w") as f:
            f.write(f"Profile = {request.form.get('profile')}\n"
                    f"Temp Critical Low = {request.form.get('temp_crit_low')}\n"
                    f"Temp Critical High = {request.form.get('temp_crit_high')}\n"
                    f"Temp Warning Low = {request.form.get('temp_warn_low')}\n"
                    f"Temp Warning High = {request.form.get('temp_warn_high')}\n"
                    f"RH Critical Low = {request.form.get('rh_crit_low')}\n"
                    f"RH Critical High = {request.form.get('rh_crit_high')}\n"
                    f"RH Warning Low = {request.form.get('rh_warn_low')}\n"
                    f"RH Warning High = {request.form.get('rh_warn_high')}\n"
                    f"Lux Critical = {request.form.get('lux_crit')}\n"
                    f"Lux Warning = {request.form.get('lux_warn')}\n"
                    f"eCO2 Critical = {request.form.get('eco2_crit')}\n"
                    f"eCO2 Warning = {request.form.get('eco2_warn')}\n"
                    f"FSM MeasurementCtrl Timer = {request.form.get('meas_t_ms')}\n"
                    f"FSM DHT11 Timer = {request.form.get('dht11_t_ms')}\n"
                    f"FSM BH1750 Timer = {request.form.get('bh1750_t_ms')}\n"
                    f"FSM CCS811 Timer = {request.form.get('ccs811_t_ms')}\n"
                    f"FSM Output Timer = {request.form.get('output_t_ms')}\n")
        return render_template("result.html")
    else:
        return 500

@app.route('/load', methods=['GET'])
def load():
    if request.method == "GET":
        s_profile = request.args.get("profileload")
        if s_profile in profiles:
            return jsonify(profiles[s_profile])
        else:
            return make_response("Profile not found",404)
    else:
        return make_response(400)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=8082)