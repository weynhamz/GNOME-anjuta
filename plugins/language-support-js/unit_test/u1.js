Function.prototype.method = function (name, func) {
    this.prototype[name] = func;
    return this;
};

function Parenizor(value) {
    this.setValue(value);
}

Parenizor.method('setValue', function (value) {
    this.value = value;
    return this;
});

Parenizor.method('getValue', function () {
    return this.value;
});

Parenizor.method('toString', function () {
    return '(' + this.getValue() + ')';
});

m= new Parenizor(0);
a = "dsa";
const GLib = imports.gi.GLib;

for (var1=0;var1<100;var1=var1+1)
{
		var1++;
}

