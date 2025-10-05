from flask import Flask, request
import logging



app = Flask(__name__)

log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)


@app.route('/LOG', methods=['GET', 'POST'])
def log():
    msg = request.args.get('msg') if request.method == 'GET' else request.form.get('msg')
    if msg:
        print(f"{msg}")
        return "Message logged", 200
    else:
        return "No msg provided", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)