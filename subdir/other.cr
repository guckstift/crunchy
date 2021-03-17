import "../test.cr"
import "more.cr"

var : u16 = 666

export func foo()
{
	print var
	var = var + 1
}

foo()
