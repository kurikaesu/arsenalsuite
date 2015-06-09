
from blur.quickinit import *

def testIntervalS( ins, out ):
	result = Interval.fromString(ins).toString()
	if( result == out ):
		print 'Passed test:', ins, '\t\t', result
	else:
		print 'Failed test:', ins, 'incorrect result was:', '"%s"' % result, 'expected:', '"%s"' % out

def testInterval( ins, out ):
	return testIntervalS( ins.toString(), out )

def ifs( s ):
	return Interval.fromString(s)

testIntervalS( '1 millennium', '1000 years' )
testIntervalS( '1.2 centuries', '120 years' )
testIntervalS( '1.6 decades', '16 years' )
testIntervalS( '1.5 years', '1 year 6 mons' )
testIntervalS( '13 months', '1 year 1 mon' )
testIntervalS( '1.5 months', '1 mon 15 days' )
testIntervalS( '1.61 months', '1 mon 18 days 07:12:00' )
testIntervalS( '1:1', '01:01:00' )
testIntervalS( '1.5:1:1', '1 day 13:01:00' )

testInterval( ifs('1 day') / 2, '12:00:00' )
