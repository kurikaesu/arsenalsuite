#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: config.py 9408 2010-03-03 22:35:49Z brobison $"
#

import sqlalchemy
import sqlalchemy.orm

engine = sqlalchemy.create_engine( 'postgres://bach:escher@sql01/bach' , echo=True)
metadata = sqlalchemy.MetaData()
Session = sqlalchemy.orm.scoped_session( sqlalchemy.orm.sessionmaker( bind=engine ) )
mapper = sqlalchemy.orm.mapper
