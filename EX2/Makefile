# DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
# Segundo Exercício de Programação
# Nome: Bernardo Reis de Almeida
# Matrícula: 2021032234

stubs: bank_stub store_stub

bank_stub: bank_pb2.py bank_pb2_grpc.py

bank_pb2.py: bank.proto
	python3 -m grpc_tools.protoc -I. --python_out=. ./bank.proto

bank_pb2_grpc.py: bank.proto
	python3 -m grpc_tools.protoc -I. --grpc_python_out=. ./bank.proto

store_stub: store_pb2.py store_pb2_grpc.py

store_pb2.py: store.proto
	python3 -m grpc_tools.protoc -I. --python_out=. ./store.proto

store_pb2_grpc.py: store.proto
	python3 -m grpc_tools.protoc -I. --grpc_python_out=. ./store.proto

run_serv_banco: bank_stub
	python3 svc_bank.py $(arg1)

run_cli_banco: bank_stub
	python3 cln_bank.py $(arg1) $(arg2)

run_serv_loja: bank_stub store_stub
	python3 svc_store.py $(arg1) $(arg2) $(arg3) $(arg4)

run_cli_loja: bank_stub store_stub
	python3 cln_store.py $(arg1) $(arg2) $(arg3)

clean:
	rm -rf bank_pb2.py bank_pb2_grpc.py store_pb2.py store_pb2_grpc.py