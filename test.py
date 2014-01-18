#!/usr/bin/python

import sys, getopt, time
import lvasr

def main(argv):
    r = 1
    try:
        opts, args = getopt.getopt(argv,"hr:")
        for opt, arg in opts:
             if opt == '-r':
                r= int(arg)
             else:
                print 'test.py -r <number of recognition runs>'
                sys.exit(0)
    except getopt.GetoptError:
        print 'test.py -r <number of recognition runs>'
        sys.exit(2)
    try:
        lvasr.init()

        if 0 != lvasr.serveravailable():
            s = lvasr.loadgrammar("label", "./demo.grxml")
            print s
            for x in xrange(r):
                start = time.time()
                t = "<sr>" + lvasr.decode("label", None, "./demo.raw") + "</sr>"
                elapsed = (time.time() - start)
                print t
                print "Decode took ", elapsed
        else:
            quit("ASR Server is not running")
    finally:
        lvasr.unloadgrammar("label")
        lvasr.uninit()

if __name__ == "__main__":
    main(sys.argv[1:])
    lvasr.uninit()