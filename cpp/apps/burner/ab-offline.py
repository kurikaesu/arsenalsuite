
import initab
host = Host.currentHost()
host.status().setSlaveStatus("stopping")
host.status().commit()

					#		    returnTasksSql += "return_slave_tasks_3(" + QString::number( h.key() ) + ")";
					#				  Database::current()->exec("SELECT " + returnTasksSql.join(",") + ";");

