# DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
# Segundo Exercício de Programação
# Nome: Bernardo Reis de Almeida
# Matrícula: 2021032234

import sys
import grpc
import bank_pb2
import threading
import bank_pb2_grpc
from concurrent import futures


# Class that implements Bank services.
class Bank(bank_pb2_grpc.BankServicer):
    def __init__(self, stop_event, accounts={}):
        self.accounts = accounts  # Stores the balance of each registered account.
        self.orders = {}  # Stores debit orders.
        self.order_count = 0  # Associates each order with a sequential ID.

        self._stop_event = stop_event  # In order to finalize server execution.

    # Returns the balance of the given account, if it exists.
    def ReadBalance(self, request, context):
        # If the account is not registered, returns -1.
        if request.account_id not in self.accounts:
            return bank_pb2.AccountResponse(value=-1)
        else:
            return bank_pb2.AccountResponse(value=self.accounts[request.account_id])

    # Registers a debit order with the given value for the given account.
    def RegisterOrder(self, request, context):
        # If the requested account does not exist, returns -1.
        if request.account_id not in self.accounts:
            return bank_pb2.OrderResponse(value=-1)
        # If the requested value is bigger than the account's balance, returns -2.
        elif request.amount > self.accounts[request.account_id]:
            return bank_pb2.OrderResponse(value=-2)
        # Debits the requested value, creates an order register and returns its ID.
        else:
            self.accounts[request.account_id] = (
                self.accounts[request.account_id] - request.amount
            )
            self.order_count = self.order_count + 1
            self.orders[self.order_count] = request.amount
            return bank_pb2.OrderResponse(value=self.order_count)

    # Executes a transference of an order to an account.
    def ExecuteTransfer(self, request, context):
        # If the requested order is not registered, returns -1.
        if request.order_id not in self.orders:
            return bank_pb2.TransferResponse(value=-1)
        # If the given amount does not match the registered amount, returns -2.
        elif request.amount_check != self.orders[request.order_id]:
            return bank_pb2.TransferResponse(value=-2)
        # If the requested account is not registered, returns -3.
        elif request.account_id not in self.accounts:
            return bank_pb2.TransferResponse(value=-3)
        # Removes the debit order and transfers the sum to the target account.
        else:
            amount = self.orders.pop(request.order_id)
            self.accounts[request.account_id] = (
                self.accounts[request.account_id] + amount
            )
            return bank_pb2.TransferResponse(value=0)

    # Finalizes the execution of the server.
    def Finalize(self, request, context):
        # Prints to stdout the id and balance of all accounts.
        for account in self.accounts:
            print(account, self.accounts[account])
        # Stops the server.
        self._stop_event.set()
        return bank_pb2.FinalizeResponse(value=len(self.orders))


# Server's operation.
def serve():
    # Reads a set of accounts from stdin.
    accounts = {}
    while True:
        try:
            entry = input()
        except EOFError:
            break  # Stops when reaching the end of the input.

        account, balance = entry.split()
        accounts[str(account)] = int(balance)

    # Receives the port through command line arguments.
    port = str(sys.argv[1])

    # Instantiates the server.
    stop_event = threading.Event()
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    bank_pb2_grpc.add_BankServicer_to_server(Bank(stop_event, accounts), server)
    server.add_insecure_port("0.0.0.0:" + port)
    server.start()
    print("Server started, listening on " + port)
    stop_event.wait()
    server.stop(None)


# Entry point of the server program.
if __name__ == "__main__":
    serve()
