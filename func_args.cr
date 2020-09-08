func foo(num1 : int, num2 : float)
{
	print "num1 =", num1, "; num2 =", num2;
}

func switcher(i : int, name : string) : string
{
	if i > 10 {
		foo(42, 3.141);
		return "Hello " + name + "!";
	}
	
	return "Is it you, " + name + "?";
}

print switcher(0, "Danny");
print switcher(100, "Thomas");
