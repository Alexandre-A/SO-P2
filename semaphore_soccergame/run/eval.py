import sys

vascos = 0
nlaus = 0


with open(sys.argv[1], "r") as file:
    txt = file.readlines()
    run = 1
    w = 0
    for line in txt:
        if f"Run n.º {run}" in line :
            if w < 3 and run > 1:
                nlaus += 1
            else:
                vascos += 1
            w = 0
            run += 1
            continue

        if "W" in line:
            w += 1
            continue

    print(f"vascos = {vascos}\nnlaus = {nlaus}")

