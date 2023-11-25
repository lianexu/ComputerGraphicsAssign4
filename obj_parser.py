lines = []
with  open("/Users/lianexu/Dropbox (MIT)/6.4400 Computer Graphics/orig_oar1.txt") as reader:
    line = reader.readline()
    while line != '':
        lines.append(line.split(' '))
        line  = reader.readline()
        

lines[0][0] = 'f'
# print(lines[0])
mod_lines = []
for ix, line in enumerate(lines):
    mod_lines.append("f")
    for i in range(1, 4):
        l = line[i].split('/')
        mod_lines[ix]+= (" " + l[0] + "/" + l[1]) 

for line in mod_lines:
    print(line)
        