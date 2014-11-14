#! /usr/bin/python

import sys
import functools
import time

from benchmark import Benchmark
from hyperdex.mongo import HyperDatabase, HyperSpace 

### CONFIG ###
# Modify this to change the behaviour of the benchmark

# How many atomic add calls?
numCalls = 1000
# How many documents in the space ? 
numDocs = 100*numCalls
# Gaps between calling 
addGap = 10
##############

def createDoc(id, numElems):
        doc = {'_id' : id}

        for i in range(numElems):
                doc['elem' + str(i)] = i

        return doc

# Verify Config
assert numCalls > 0
assert numDocs > 0
assert addGap > 0
assert (numCalls * addGap) < numDocs

# Setup
db = HyperDatabase(sys.argv[1], int(sys.argv[2]))

# Run
for n in range(500, 10000, 250):
        pendingInserts = []

        for i in range(numDocs):
                p = db.bench.async_insert(createDoc('myval' + str(i), n))
                pendingInserts.append(p)

        while len(pendingInserts) > 0:
                pendingInserts[0].wait()
                pendingInserts.pop(0)
        
        time.sleep(1)
        
        pendingOps = []

        bench = Benchmark()
        bench.start()
        
        for i in range(numCalls):
                p = db.bench.async_atomic_add('myval' + str(i*addGap), {'elem50' : 10})
                pendingOps.append(p)
        
        while len(pendingOps) > 0:
                pendingOps[0].wait()
                pendingOps.pop(0)

        elapsed = bench.stop()

        # Print simple CSV
        print str(n) + ',' + str(elapsed)

        db.bench.clear()
