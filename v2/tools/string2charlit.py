# this script transforms strings from input line
# "asdf" to 'a', 's', 'd', 'f'
# which are useful for pegtl library


while True:
    line = raw_input("string to charize: ")
    if len(line) == 0:
        exit(0)

    res = []
    for c in line:
        res.append("'%s'" % c)
    print(", ".join(res))