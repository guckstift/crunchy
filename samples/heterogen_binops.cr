var f:float;

f = 9.1;

f = f + 8;
f = 1 + f;
f = 1.2 + 5.2;
f = f + true;

print f;

var i:int;

i = 1 + false;
i = true - 21;
i = true + true;

print i;

var b :bool;

#b = true + false;

f = 1.5 + 1 + true + 2 - 0.3 + false;

print f;

print 1 != 1.3;
print 1 != false;
print true > false;
print true < false;

var ss = "H" + "";

var tt = "" + "H" + "";

print "H" == ss;

# print "9" == 9; # can not combine string and int with the operator "=="
# print true == "true"; # can not combine bool and string with the operator "=="

var x = "H" == ss;
var y = 9 == 9;
