
import "subdir/other.cr"

b := [1,2]
a := >b

#c := <a

#a = [1,2]

i := 9
#c := [i, i]

#x := [1, [2, 3]]

func foo2() : >[2]int
{
	return b
}

m := [[1,2],[3,4], [3,90]]

