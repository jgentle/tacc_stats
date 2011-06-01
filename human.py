# Based on coreutils human_readable()

def human_readable(amt):
    amt = int(amt)
    fraction = ""
    prefix_letters = "KMGTPEZY"
    exponent_max = len(prefix_letters)
    exponent = 0
    base = 1024
    tenths = 0
    rounding = 0
    if base <= amt:
        while base <= amt and exponent < exponent_max:
            r10 = 10 * (amt % base) + tenths
            r2 = 2 * (r10 % base) + (rounding >> 1)
            amt /= base
            tenths = r10 / base
            if r2 < base:
                if r2 + rounding != 0:
                    rounding = 1
                else:
                    rounding = 0
            else:
                if base < r2 + rounding:
                    rounding = 3
                else:
                    rounding = 2
            exponent += 1
        if amt < 10:
            if 2 < rounding + (tenths & 1):
                tenths += 1
                rounding = 0
                if tenths == 10:
                    amt += 1
                    tenths = 0
            if amt < 10 and tenths != 0:
                fraction = "." + str(tenths)
                tenths = rounding = 0
    delta = 0
    if 0 < rounding + (amt & 1):
        delta = 1
    if 5 < tenths + delta:
        amt += 1
        if amt == base and exponent < exponent_max:
            exponent += 1
            fraction = ".0"
            amt = 1
    prefix = ""
    if exponent > 0:
        prefix = prefix_letters[exponent - 1]
    return str(amt) + fraction + prefix
