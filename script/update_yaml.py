import os
import sys
import yaml

father = sys.argv[1]
key = sys.argv[2]
value = sys.argv[3]

path = "config/"
files = [path+i for i in os.listdir(path)]
print files
for f in files:
    print f
    s = open(f).read()
    print s
    temp = yaml.load(s)
    print temp
    if temp.get(father)!=None and temp.get(father).get(key) != None:
        temp[father][key] = value
    else:
        temp[father].update({key:value})
    f=open(f,'w')
    f.write(yaml.dump(temp))
    f.close()
