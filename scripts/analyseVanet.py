import sys
from optparse import OptionParser
import fileinput
from scipy import spatial
from numpy import * 

parser = OptionParser("usage: %prog [options]")
parser.add_option('--vanetFile', help=("VANET file"), type="string", dest='vanetFile')
parser.add_option('--fromNS3', help=("If the vanet file was generated by NS3 (contains MAC address)"), action='store_true', default=False, dest="fromNS3" )
(options, args) = parser.parse_args()

print options

# check set options
if not options.vanetFile:
	print "Usage: analyseVanet --vanetFile <FILE> [--fromNS3]" 
	print "Exiting..."
	exit()

macMap = {}
currentStep = -1
points = []
stepVehicles = []
stepEdges = {}
stepEdgesCount = 0

def populateMacMap(elem, args={}):
	#print line	
	global macMap
	elem = line.split(',')
	if len(elem) > 5:
		vehicleId = str(elem[2])
		vehicleMac = str(elem[5])
		macMap[vehicleMac] = vehicleId
	return 	

def calculateDistance(p0, p1):
	deltaX = p0[0] - p1[0]
	deltaY = p0[1] - p1[1]
	# print "deltaX: {}, deltaY: {}".format(deltaX, deltaY)
	distance = math.sqrt((p0[0] - p1[0])*(p0[0] - p1[0]) + (p0[1] - p1[1])*(p0[1] - p1[1]))
	return distance

def getEdgesWeight(edges, currentStep):
	connections = {}
	distances = []

	for (vehicleId, neighbors) in edges.iteritems():
		vehiclePointId = stepVehicles.index(vehicleId)
		a = points[int(vehiclePointId)]
		connections[vehicleId] = {}
		connections[vehicleId]['distances'] = []
		for neighborId in neighbors:
			# distance = pdist(X, 'euclidean')
			neighborPointId = stepVehicles.index(neighborId)
			b = points[int(neighborPointId)]
			distance = calculateDistance(a,b)
			# print "{} calculating distance between {}{} and {}{} = {}".format(currentStep, vehicleId, a, neighborId, b, distance)
			connections[vehicleId]['distances'].append(distance)
			distances.append(distance)

	if distances:
		sumDistance = sum(distances)
		numberOfConnections = len(distances)
		maxDistance = max(distances)
		print "currentStep {}: numberOfConnections: {}, avgDistance: {}, maxDistance: {}".format(currentStep, numberOfConnections, sumDistance/numberOfConnections, maxDistance)
		

def processLine(line, args={}):	
	global macMap
	global stepVehicles
	global stepEdges
	global currentStep
	global points
	global stepEdgesCount

	#1 21631 21 23094 21968.5 00:00:00:00:00:16 2 00:00:00:00:00:20,1 00:00:00:00:01:f7,1 
	elem = line.split(',')
	step = float(elem[0])
	time = float(elem[1])
	vehicleId = str(elem[2])
	vehicleX = float(elem[3])
	vehicleY = float(elem[4])
	vehicleMac = str(elem[5])
	vehicleNumberOfEdges = int(elem[6])
	vehicleEdges = []
	for i in range(0,vehicleNumberOfEdges):
		neighborId = str(elem[7+i]).strip()
		if options.fromNS3:
			neighborId = macMap[elem[7+i]]
		vehicleEdges.append(neighborId)

	if step != currentStep:	
		print "currentStep: {},  vehicles: {}, edges: {} ".format(currentStep, len(stepVehicles), stepEdgesCount)
		getEdgesWeight(stepEdges, currentStep)
		currentStep = step
		stepVehicles = []
		stepEdges = {}
		stepEdgesCount = 0
		points = []

	points.append([vehicleX, vehicleY])
	stepVehicles.append(vehicleId)
	stepEdges[vehicleId] = vehicleEdges
	stepEdgesCount += len(vehicleEdges)
	
	return 0



#################

if options.fromNS3:
	for line in fileinput.input(options.vanetFile):
		populateMacMap(line, args)
for line in fileinput.input(options.vanetFile):
	processLine(line, args)

# tree = spatial.KDTree(points, 10)
# pairs = []
# i = 0
# for point in points:
# 	neighbors = tree.query_ball_point(point, radious)
# 	x = point[0]
# 	y = point[1]
# 	neighbors.remove(i)
# 	numberOfNeighbors = len(neighbors)
# 	pairs.extend(neighbors)
# 	# print "neighbors of "+str(i)+"("+str(x)+','+str(y)+"): "+str(neighbors) 
# 	i = i+1

# 	print "step\t" + str(time) + " stepVehicles \t" + str(len(stepVehicles)) + ', edges: ' + str(len(pairs))

