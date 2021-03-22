
a := [1,2,3]
b := >a
c := b[0]
d := >b[1]

a[1] = 9
b[2] = 3

func foo() : >[3]int
{
	return a
}

x := -foo()[2]

print x

print 15 % 10 * 100
