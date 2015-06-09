#ifndef _Bach_utils_h_
#define _Bach_utils_h_

//#define _DO_DEBG
#ifdef _DO_DEBG
#define DEBG(X) { QString sss = X; qWarning() << QString( __FILE__ ) << QString::number( __LINE__ ) << sss; }
#else
#define DEBG(X)
#endif

#define PRNT(X) { QString sss = X; qWarning() << QString( __FILE__ ) << QString::number( __LINE__ ) << sss; }

//-----------------------------------------------------------------------------
inline void SmartTokenise(	const QString & a_Str,
							QStringList & o_Tokens,
							const QString & a_Splitters = " \t",
							const QString & aQuoteChars = "\"" )
{
	QString tokenWIP;
	QString::const_iterator quoteStartChar = a_Str.end();
	for ( QString::const_iterator aChar = a_Str.begin() ; aChar != a_Str.end() ; ++aChar )
	{
		if ( quoteStartChar != a_Str.end() )
		{
			if ( *aChar == *quoteStartChar )
			{
				quoteStartChar = a_Str.end();
				if ( !tokenWIP.isEmpty() ) o_Tokens.push_back( tokenWIP );
				tokenWIP.clear();
			}
			else
			{
				tokenWIP.push_back( *aChar );
			}
		}
		else
		{
			if ( a_Splitters.contains( *aChar ) )
			{
				if ( !tokenWIP.isEmpty() )
				{
					o_Tokens.push_back( tokenWIP );
					tokenWIP.clear();
				}
			}
			else if ( aQuoteChars.contains( *aChar ) )
			{
				quoteStartChar = aChar;
				if ( !tokenWIP.isEmpty() )
				{
					o_Tokens.push_back( tokenWIP );
					tokenWIP.clear();
				}
			}
			else
			{
				tokenWIP.push_back( *aChar );
			}
		}
	}
	if ( !tokenWIP.isEmpty() )
	{
		o_Tokens.push_back( tokenWIP );
	}
}
#endif // _Bach_utils_h_
