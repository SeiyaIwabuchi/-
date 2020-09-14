from flask import Flask
from flask.globals import request
app = Flask(__name__)

dataDic = {}

@app.route("/")
def index():
    return "dataDic={}<br>{}<br>{}".format(
        str(dataDic),
        "http://{}/[keyName]/get".format(request.headers["host"]),
        "http://{}/[keyName]/set/[value]".format(request.headers["host"])
        )

@app.route("/<key>")
def getKeyValue(key=None):
    try:
        return dataDic[key]
    except KeyError:
        return "key:{} if not found.".format(str(key))

@app.route("/<key>/get")
def getKeyValue2(key=None):
    return getKeyValue(key)

@app.route("/<key>/set/<value>")
def setKeyValue(key=None,value=None):
    dataDic[key] = value
    return getKeyValue(key)

if __name__ == "__main__":
    app.run(debug=True,host="0.0.0.0",port=1919)