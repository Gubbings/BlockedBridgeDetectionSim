import os
import subprocess
from datetime import datetime

testSetup = {
	"binName" 							: "simulation.exe"
    , "binDir" 							: "bin"
    , "dataDir" 						: "data"
    , "numTrials"						: 1    
    , "iterationCount" 					: "1000"
    , "hoursPerUpdates" 				: "12"
    , "totalUsers" 						: "10000"
    , "initBridgeCounts" 				: "3500"
    , "blockChancePercents" 			: ["0.05"]
    , "reportChancePercents" 			: ["50"]
    , "maxSingleUserBridgeAccessPerTimeIntervals" : ["100"]
    , "minSingleUserBridgeAccessPerTimeIntervals" : ["20"]
    , "geoIPErrorChancePercents" 		: ["10"]
    # , "regionList" 						: "ca,ru,cn,us"
    , "regionList" 						: "cn"
    , "censorRegionList" 				: "cn"
    , "reportThresholds" 				: ["5"]
    , "bridgeStatUsageDiffThresholds" 	: ["50"]
    , "bridgeMessageDropChancePercent" 	: ["10"]
    , "numberOfDaysForAvgBridgeStats" 	: ["7"]
    # , "percentUsersPerRegion" 			: "10,10,70,10"
    , "percentUsersPerRegion" 			: "100"
    , "minBridgeDBSize" 				: "1000"
    , "nonSusBridgeProbeChancePerecent" : "5"
    , "reportWeights" 					: ["0.4"]
    , "bridgeStatsDiffWeight" 			: ["0.9"]
    , "minConfidenceToProbe" 			: ["0.9"]
    , "minBridgeUsageThreshold" 		: ["32"]
    , "probeChancePercent" 				: ["90"]
    , "numRetriesPerProbe" 				: ["10"]
    , "reuseSameSrandSeed" 				: False
    , "srandSeed"						: ""
}

dataItemCount = 0

def runSingleTest(dataDirFullPath, binDir, itCount, hpu, totalUsers, initBridgeCount, blkChance, repChance, maxBA, minBA, geoE, regionList, censorList, reportT, bstatT, messDropC, bstatAvgDays, usersPerRegion, minBDB, reportW, bstatsW, minConfToProbe, minBUseT, probeChance, retriesPerProbe, srandSeed):
	# os.chdir(binDir)	
	global dataItemCount
	cmd = "cd " + binDir + " ; ./" + testSetup["binName"] + " -i " + itCount + " -hpu " + hpu + " -u " + totalUsers + " -b " + initBridgeCount + " -blkChance " + blkChance + " -rChance " + repChance + " -maxBA " + maxBA + " -minBA " + minBA + " -ge " + geoE + " -regions " + regionList + " -censors " + censorList + " -rt " + reportT + " -bst " + bstatT + " -dropChance " + messDropC + " -bsAvgDays " + bstatAvgDays + " -upr " + usersPerRegion + " -minBDB " + minBDB + " -pChance " + probeChance + " -rw " + reportW + " -bsdw " + bstatsW + " -mctp " + minConfToProbe + " -but " + minBUseT + " -rpp " + retriesPerProbe + " -nsbpc " + testSetup["nonSusBridgeProbeChancePerecent"]
	if testSetup["reuseSameSrandSeed"]:
		cmd = cmd + " -seed " + srandSeed
	# cmd = cmd + " | tee " + dataDirFullPath + "/data" + str(dataItemCount)
	cmd = cmd + " >> " + dataDirFullPath + "/data" + str(dataItemCount) + ".txt 2>&1"
	print(cmd)
	# splitCmd = cmd.split()
	# print(splitCmd)
	print("Starting run " + str(dataItemCount))
	# subprocess.run(splitCmd, capture_output=True)
	child = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
	# childprocess = subprocess.Popen([cmd], stdout=subprocess.PIPE, shell=True)
	returncode = child.returncode
	ret_stdout = child.stdout.decode('utf-8').rstrip(' \r\n')
	print(ret_stdout)
	# (out, err) = childprocess.communicate()
	# print(out)
	# out, err = childprocess.communicate()
	dataItemCount = dataItemCount + 1	
	print("Done")

def runTrials(dataDirFullPath, srandSeed):
	itCount = testSetup["iterationCount"]
	minBDB = testSetup["minBridgeDBSize"]
	regList = testSetup["regionList"]
	cenList = testSetup["censorRegionList"]
	userPerReg = testSetup["percentUsersPerRegion"]
	totalUsers = testSetup["totalUsers"]
	hpu = testSetup["hoursPerUpdates"]
	initBridgeCount = testSetup["initBridgeCounts"]

	for blockChanceP in testSetup["blockChancePercents"]:
		for reportChanceP in testSetup["reportChancePercents"]:
			for maxBA in testSetup["maxSingleUserBridgeAccessPerTimeIntervals"]:
				for minBA in testSetup["minSingleUserBridgeAccessPerTimeIntervals"]:
					for geoE in testSetup["geoIPErrorChancePercents"]:
						for repT in testSetup["reportThresholds"]:
							for bstatsT in testSetup["bridgeStatUsageDiffThresholds"]:
								for messDropC in testSetup["bridgeMessageDropChancePercent"]:
									for bstatsAvgDays in testSetup["numberOfDaysForAvgBridgeStats"]:
										for repW in testSetup["reportWeights"]:
											for bstatsW in testSetup["bridgeStatsDiffWeight"]:
												for minConfToProbe in testSetup["minConfidenceToProbe"]:
													for minBUseT in testSetup["minBridgeUsageThreshold"]:
														for probeC in testSetup["probeChancePercent"]:
															for probeRetries in testSetup["numRetriesPerProbe"]:
																runSingleTest(dataDirFullPath, testSetup["binDir"], itCount, hpu, totalUsers, initBridgeCount, blockChanceP, reportChanceP, maxBA, minBA, geoE, regList, cenList, repT, bstatsT, messDropC, bstatsAvgDays, userPerReg, minBDB, repW, bstatsW, minConfToProbe, minBUseT, probeC, probeRetries, srandSeed)

def main():
	print("Running tests")
	
	if not os.path.isdir(testSetup["dataDir"]):
		subprocess.Popen(["mkdir " + testSetup["dataDir"]], stdout=subprocess.PIPE, shell=True)
	
	now = datetime.now()
	dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")
	currentDataDir = testSetup["dataDir"] + "/" + dt_string
	process = subprocess.Popen(["mkdir " + currentDataDir], stdout=subprocess.PIPE, shell=True)
	process.communicate()
	dataDirFullPath = os.path.join(os.getcwd(), currentDataDir)

	srandSeed=""
	if testSetup["reuseSameSrandSeed"]:
		srandSeed=testSetup["srandSeed"]

	for i in range (0, testSetup["numTrials"]):
		runTrials(dataDirFullPath, srandSeed)


if __name__ == "__main__":	
	childprocess = subprocess.Popen(["pwd"], stdout=subprocess.PIPE, shell=True)
	(out, err) = childprocess.communicate()
	print("Current working directory = ", out.decode('utf8', errors='strict').strip())
	main()
	# childprocess = subprocess.Popen(["cd bin ; ./simulation.exe"], stdout=subprocess.PIPE, shell=True)
	# (out, err) = childprocess.communicate()
	# print(out)
