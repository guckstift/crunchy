
func bar() : bool
{
	return true;
}

func foo() : int
{
	return bar();
}

var x : string = foo();

print x;


var f : float = 3.141592653589793;

print f, "is a float";
print 0.3, "is a float";

var s : string = f;

print s;


func foo2() : float
{
	return 0.4;
}

s = foo2();
print s;

f = true;

print f;

#var i:int = 1.0;


