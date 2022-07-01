# load
import dueca

ps = dueca.PrioritySpec(0,0)

m = dueca.Module("mtype", "part", ps)

m.param(b=5,c=5,a=3).param(a=3)
