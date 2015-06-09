
_DEBUG_ = False

import re
from PyQt4.QtCore import QVariant, QString
from blur.Stone import *

class FieldMethodObject:
	def __init__(self,**kw):
		self.__dict__ = kw

# Function object that calls self.Method and casts the result to self.Class
class CastReturnMethodObject(FieldMethodObject):
	def __call__(self,object, *args):
		return self.Class(self.Method(object,*args))

# Used for Record getters, function object knows the field that is being retrieved, and can
# converts the results from QVariants to python types
class GetMethodObject(FieldMethodObject):
	def __call__(self,object,fail=None):
		if self.Field.foreignKeyTable():
			ret = Record.foreignKey(object,self.Field.name())
			try:
				ret = self.ClassDict[str(self.Field.foreignKeyTable().className())](ret)
			except:
				if ( _DEBUG_ ):
					print self.ClassDict
					print ("Unable to find table %s in class dictionary" % self.Field.foreignKeyTable().className())
			return ret
		out = wrapQVariant(Record.getValue(object,self.Field.name()), False, True, self.Field.qvariantType())
		if ( out == None ):
			out = fail
		return out

# Used for RecordList getters.
class ListGetMethodObject(FieldMethodObject):
	def __call__(self,object):
		if ( _DEBUG_ ):
			print "ListGetMethodObject.__call__"
			
		if self.Field.foreignKeyTable():
			return object.foreignKey(self.Field.name())
		return variantListToPyList(object.getValue(self.Field.name()),self.Field.qvariantType())

# Used for record setters
class SetMethodObject(FieldMethodObject):
	def __call__(self,object,value):
		if self.Field.foreignKeyTable():
			return self.TableClass(Record.setForeignKey(object,self.Field.name(),value))
		else:
			return self.TableClass(Record.setValue(object,self.Field.name(),QVariant(value)))

# Used for RecordList setters
class ListSetMethodObject(FieldMethodObject):
	def __call__(self,object,value):
		ret = object.__class__()
		value = QVariant(value)
		for i in range(len(object)):
			rec = object[i]
			if self.Field.foreignKeyTable():
				ret.append(Record.setForeignKey(rec,self.Field.name(),value))
			else:
				ret.append(Record.setValue(rec,self.Field.name(),value))
		return ret

# function object for RecordList subclasses' __init__ function
class ListInitMethodObject(FieldMethodObject):
	def __call__(self,object,*args):
		if len(args):
			self.ParentType.__init__(object,args[0])
		else:
			self.ParentType.__init__(object)

# Used for static Record subclass select function, casts the return 
# record list to recordlist subclass
class SelectMethodObject(FieldMethodObject):
	def __call__(self,where = QString(), args = []):
		return self.ListType(self.TableSchema.table().select(where,args))

# Static table function object
class TableMethodObject(FieldMethodObject):
	def __call__(self):
		return self.TableSchema.table()

class TableSchemaMethodObject(FieldMethodObject):
	def __call__(self):
		return self.TableSchema

def setMethodName(methodName):
	if ( re.match( 'is[A-Z]', str(methodName) ) ):
		return 'set' + str(methodName).lstrip( 'is' ).capitalize()
	else:
		return 'set' + methodName[0].toUpper() + methodName[1:]

class InitMethodObject(FieldMethodObject):
	def __call__(self,object,*args):
		arg = None
		if len(args):
			if issubclass(type(args[0]),Record) and args[0].table():
				if ( _DEBUG_ ):
					print args[0]
					
				argSchema = args[0].table().schema()
				# Check to make sure it's a subclass of this type
				if self.TableSchema.isDescendant(argSchema):
					arg = args[0]
			else:
				arg = args[0]
		if arg is None:
			arg = self.TableSchema.table().emptyImp()
		self.ParentType.__init__(object,arg)

class IndexMethodObject(FieldMethodObject):
	def __call__(self,*args):
		qvargs = []
		for arg in args:
			if isinstance(arg,Record):
				arg = arg.key()
				if arg == 0: arg = QVariant(QVariant.Int)
			qvargs.append(QVariant(arg))
				
		index = self.IndexSchema.table().table().indexFromSchema(self.IndexSchema)
		vals = index.recordsByIndex(qvargs)
		if self.IndexSchema.holdsList():
			return self.ListClass(vals)
		if len(vals):
			return self.TableClass(vals[0])
		return self.TableClass()

class ReverseAccessMethodObject(FieldMethodObject):
	def __call__(self,object):
		vals = self.Field.table().table().indexFromSchema( self.Field.index() ).recordsByIndex( [QVariant(object.key())] )
		if self.Field.flag(Field.Unique):
			return self.TableClass( vals[0] )
		return self.ListClass( vals )

class RecordListOverload(RecordList):
	def __getitem__(self,key):
		return self.__class__.ClassType(RecordList.__getitem__(self,key))
	def __add__(self,key):
		return self.__class__(RecordList.__add__(self,key))
	def __sub__(self,key):
		return self.__class__(RecordList.__sub__(self,key))
	def __and__(self,key):
		return self.__class__(RecordList.__and__(self,key))
	def __or__(self,key):
		return self.__class__(RecordList.__or__(self,key))
	def __iadd__(self,key):
		return self.__class__(RecordList.__iadd__(self,key))
	def __isub__(self,key):
		return self.__class__(RecordList.__getitem__(self,key))
	def __iand__(self,key):
		return self.__class__(RecordList.__and__(self,key))
	def __ior__(self,key):
		return self.__class__(RecordList.__or__(self,key))

from new import instancemethod

def createTableType( tableSchema, dict = None ):
	className 		= str(tableSchema.className())
	listClassName 	= str(className + 'List')
	
	parentType = Record
	parentListType = RecordListOverload
	
	if tableSchema.parent() and str(tableSchema.parent().className()) in dict:
		parentName = str(tableSchema.parent().className())
		parentType = dict[parentName]
		parentListType = dict[parentName + 'List']
	
	createdTableType = False
	createdListType = False
	
	if className + 'Base' in dict:
		tableType = type(className + '_', (dict[className+'Base'],),{'__doc__':'Dynamically generated class'})
	elif className in dict:
		dict[className].__bases__ = (parentType,)
		tableType = type(className + '_', (dict[className],),{'__doc__':'Dynamically generated class'})
	else:
		# This creates the new type, we still have to add all the methods
		tableType = type(className, (parentType,),{'__init__':instancemethod(InitMethodObject(TableSchema=tableSchema,ParentType=parentType),None,parentType)})
		createdTableType = True
	
	if listClassName in dict:
		dict[listClassName].__bases__ 				= (parentListType,)
		listType = type(listClassName + '_', (dict[listClassName],), {'__doc__':'Dynamically generated class'})
		listType.__dict__[ 'ClassType' ] 			= tableType
	else:
		# Creates the new list type
		listType = type(listClassName, (parentListType,), {'__init__':instancemethod(ListInitMethodObject(ParentType=parentListType),None,parentListType)})
		createdListType = True
	
	ttd = tableType.__dict__
	if createdTableType:
		ttd['table'] = TableMethodObject(TableSchema=tableSchema)
		ttd['schema'] = TableSchemaMethodObject(TableSchema=tableSchema)
		ttd['select'] = SelectMethodObject(TableSchema=tableSchema,ListType=listType)

	ltd = listType.__dict__
	if createdListType:
		ltd['table'] 		= ttd['table']
		ltd['schema'] 		= ttd['schema']
		ltd['ClassType'] 	= tableType
		
		# Add cast methods
		for method in ['filter','sorted','unique','reversed','reloaded']:
			ltd[method] = instancemethod(CastReturnMethodObject(Method=getattr(RecordList,method),Class=listType),None,listType)
		
	# Getters and setters
	# Add each only if they don't already exist
	for field in tableSchema.ownedFields():
		atn = str(field.methodName())
		if not hasattr(tableType,atn):
			ttd[atn] = instancemethod(GetMethodObject(Field=field,ClassDict=dict),None,tableType)
		atn = str(setMethodName(field.methodName()))
		if not hasattr(tableType,atn):
			ttd[atn] = instancemethod(SetMethodObject(Field=field,TableClass=tableType),None,tableType)
		atn = str(field.pluralMethodName())
		if not hasattr(listType,atn):
			ltd[atn] = instancemethod(ListGetMethodObject(Field=field,ClassDict=dict),None,listType)
		atn = str(setMethodName(field.pluralMethodName()))
		if not hasattr(listType,atn):
			ltd[atn] = instancemethod(ListSetMethodObject(Field=field),None,listType)
	
	# Index functions
	for indexSchema in tableSchema.indexes():
		if indexSchema.field() and indexSchema.field().flag(Field.PrimaryKey): continue
		prefix = 'recordBy'
		if indexSchema.holdsList():
			prefix = 'recordsBy'
		name = str(prefix + indexSchema.name()[0].toUpper() + indexSchema.name()[1:])
		if not hasattr(tableType,name):
			ttd[name] = IndexMethodObject(IndexSchema=indexSchema,ClassDict=dict,TableClass=tableType,ListClass=listType)
	
	# Put the new types in the dict
	if dict != None:
		dict[className] = tableType
		dict[listClassName] = listType
		
	return (tableType,listType)

def createReverseAccessors(tableSchema,typeDict):
	name = tableSchema.className()
	name = str(name[0].toLower() + name[1:])
	for field in tableSchema.ownedFields():
		if field.flag(Field.ReverseAccess):
			fkt = field.foreignKeyTable()
			if fkt in typeDict:
				methodName = name
				if not field.flag(Field.Unique):
					methodName = str(pluralizeName(methodName))
				tableType,listType = typeDict[tableSchema]
				if ( _DEBUG_ ):
					print "Adding Reverse Accessor %s.%s" % (fkt.className(),methodName)
				objectTableType = typeDict[fkt][0]
				if not hasattr(objectTableType,methodName):
					objectTableType.__dict__[methodName] = instancemethod(ReverseAccessMethodObject( Field=field, ListClass=listType, TableClass=tableType ), None, objectTableType)
				else:
					if ( _DEBUG_ ):
						print "%s already in dict" % methodName
			else:
				if ( _DEBUG_ ):
					print "Couldn't find %s in type dict" % fkt

def createTableTypes( tables, dict = None ):
	if dict is None: dict = globals()
	typeDict = {}
	
	origtables = tables[:]
	
	while True:
		childrenDoLater = []
		for table in tables:
			if table.parent() and (table.parent() in tables) and table.parent() not in typeDict:
				childrenDoLater.append(table)
				continue
				
			if ( _DEBUG_ ):
				print "Generating ", table.className()
				
			typeDict[table] = createTableType( table, dict )
		tables = childrenDoLater
		if len(tables)==0: break
	
	for table in origtables:
		createReverseAccessors(table,typeDict)
