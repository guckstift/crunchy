i := 9
p := >i
pp := >p

# adjust_assign_target
p = i  # <p = i
pp = p # <pp = p
pp = i # <<pp = i

# adjust_assign_value
i = p  # i = <p
i = pp # i = <<p
p = pp # p = <pp

# adjust_init_value
p2 : >int = i    # p2 = >i
pp2 : >>int = p  # pp2 = >p
pp3 : >>int = pp # pp3 = pp

# adjust_assign_value
i2 : int = p   # i2 = <p
i3 : int = pp  # i3 = <<pp
p3 : >int = pp # p3 = <pp

pp3 = <<pp

func foo() : >int
{
	return pp
}
