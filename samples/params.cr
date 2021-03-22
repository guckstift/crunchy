
a := [1,2]

i := 9
p := >i

print foo(p, i, a)

func foo(x : int, y : >int, z : >[2]int) : int
{
	return x * <y
}

