import os
import sys
import yaml

father = sys.argv[1]
key = sys.argv[2]
value = sys.argv[3]
if value.isdigit():
    value = int(value)
false=0
true=1


path = "config/"
files = [path+i for i in os.listdir(path)]

if father != '0':
    for f in files:
        s = open(f).read()
        temp = yaml.load(s)
        if temp.get(father)!=None and temp.get(father).get(key) != None:
            temp[father][key] = value
        else:
            temp[father].update({key:value})
        f=open(f,'w')
        f.write(yaml.dump(temp))
        f.close()
def inner(stru):
    global key,value     
    if isinstance(stru,dict):
        if stru.get(key)!=None:
            stru[key]=value
            return true
    if isinstance(stru,str):
        return false
    if isinstance(stru,dict):
        for i in stru:
            res = inner(stru[i])
            if res == true:
                return true
    if isinstance(stru,list):
        for i in range(len(stru)):
            res = inner(stru[i])
            if res == true:
                return true
if father == '0':
        for f in files:
            s = open(f).read()
            temp = yaml.load(s)
            inner(temp)
            f=open(f,'w')
            f.write(yaml.dump(temp))
            f.close()
    
