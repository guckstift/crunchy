i := 9
p := >i
pp := >p

p = i  # <p = i
pp = p # <pp = p
pp = i # <<pp = i

i = p  # i = <p
i = pp # i = <<p
p = pp # p = <pp

p2 : >int = i    # p2 = >i
pp2 : >>int = p  # pp2 = >p
pp3 : >>int = pp # pp3 = pp

i2 : int = p   # i2 = <p
i3 : int = pp  # i3 = <<pp
p3 : >int = pp # p3 = <pp

pp3 = <<pp
