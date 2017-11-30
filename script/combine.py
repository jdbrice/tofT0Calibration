#!/opt/local/bin/python

from __future__ import print_function
import sys

if len(sys.argv) < 3 :
	print( "Need 2 filenames" )
	exit()


fname1 = sys.argv[1]
fname2 = sys.argv[2]

print( "Combining", fname1, "and", fname2 )


lines1 = [line.rstrip('\n') for line in open( fname1 )]
lines2 = [line.rstrip('\n') for line in open( fname2 )]

print( "File1 has length ", len(lines1) )
print( "File2 has length ", len(lines2) )

if len(lines1) != len(lines2) :
	print ( "Wrong number of lines! Double check " )
	exit()


with open( 'combined_t0_4DB.dat', 'w' ) as fo :
	counter = 0
	cid = ""
	for l1, l2 in zip( lines1, lines2 ) :
		if counter % 2 == 0 :
			# print( "id ", l1 )
			cid = l1
		else :
			fo.write( cid + "\n" )
			nVal = float( l1 ) + float(l2)
			fo.write( str(nVal) + "\n" )
		counter = counter + 1

