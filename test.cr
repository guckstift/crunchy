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
