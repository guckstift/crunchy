import load_source from "./loader.cr"

if crunchy.args.length < 2 {
	# error
}

src_name = crunchy.args[1]
src = load_source(src_name)
