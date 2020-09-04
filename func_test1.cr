
func foo()
{
	var s = "";
	var i = 10;
	
	while i > 0 {
		print i;
		i = i - 1;
		s = s + "*";
		print s;
	}
	
	return;
}

foo();

print "Nice!" + " Danny";

foo();

func bar() : int
{
	return 0;
}
