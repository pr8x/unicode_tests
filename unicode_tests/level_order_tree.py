
# int eytzinger(int i = 0, int k = 1) {
#     if (k <= n) {
#         i = eytzinger(i, 2 * k);
#         b[k] = a[i++];
#         i = eytzinger(i, 2 * k + 1);
#     }
#     return i;
# }

def eytzinger(i, k, a, b):
    if k <= len(a):
        i = eytzinger(i, 2 * k, a, b)
        b[k] = a[i]
        i += 1
        i = eytzinger(i, 2 * k + 1, a, b)
    return i

if __name__ == '__main__': 
    with open('grapheme_break_data.txt') as f:
        a = f.read().splitlines()
        b = [0]* (len(a) + 1)
        eytzinger(0, 1, a, b)
    with open('grapheme_break_data_lo.txt', 'w') as f:
        for item in b:
            f.write("{}\n".format(item))

      