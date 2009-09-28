Function.prototype.method = function (name, func) {
    this.prototype[name] = func;
    return this;
};

function Parenizor(value) {
    this.setValue(value);
}

mazx = new Parenizor(0);
aasb = "dsa";
const GLib = imports.gi.GLib;

