./darwin_from_sg.sh | psql -h farmdb -U farmer blur -1 -c "DELETE FROM darwinweight; COPY darwinweight (shotname, weight, projectname) FROM STDIN"
