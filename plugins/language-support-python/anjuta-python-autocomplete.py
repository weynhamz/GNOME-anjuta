import getopt
import sys
from rope.base.project import Project 
from rope.contrib import codeassist
from rope.contrib import autoimport
import os

def pathsplit(p, rest=[]):
	(h,t) = os.path.split(p)
	if len(h) < 1: return [t]+rest
	if len(t) < 1: return [h]+rest
	return pathsplit(h,[t]+rest)

def commonpath(l1, l2, common=[]):
	if len(l1) < 1: return (common, l1, l2)
	if len(l2) < 1: return (common, l1, l2)
	if l1[0] != l2[0]: return (common, l1, l2)
	return commonpath(l1[1:], l2[1:], common+[l1[0]])

def relpath(p1, p2):
	(common,l1,l2) = commonpath(pathsplit(p1), pathsplit(p2))
	p = []
	if len(l1) > 0:
	    p = [ '../' * len(l1) ]
	p = p + l2
	return os.path.join( *p )


options, remainder = getopt.getopt(sys.argv[1:], 'o:p:s:r:f:')

for opt, arg in options:
    if opt in ('-o', '--option'):
        option_arg = arg
    elif opt in ('-p', '--project'):
        project_arg = arg
    elif opt == '-s':
        source_code_arg = arg
    elif opt == '-r':
        res_arg = arg
    elif opt == '-f':
        offset_arg = arg

option = option_arg;
projectpath = project_arg
if projectpath.startswith("file://"):
	projectpath = projectpath.replace("file://", "")

proj = Project(projectpath)
proj.pycore._init_python_files()

input = open(source_code_arg, 'r')
source_code = input.read()
respath = relpath(projectpath, res_arg)
res = proj.get_resource(respath)

position = int(offset_arg)

try:
	if option == "autocomplete":
		proposals = codeassist.code_assist(proj, source_code, position, resource=res, maxfixes=10)
		proposals = codeassist.sorted_proposals(proposals)

		for proposal in proposals:
			print proposal

	elif option == "calltip":
		proposals = codeassist.get_doc(proj, source_code, position, resource=res, maxfixes=10)
		print proposals
except:
	pass

proj.close()
