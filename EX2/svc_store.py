# DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
# Segundo Exercício de Programação
# Nome: Bernardo Reis de Almeida
# Matrícula: 2021032234

import sys
import grpc
import bank_pb2
import store_pb2
import threading
import bank_pb2_grpc
import store_pb2_grpc
from concurrent import futures


# Class that implements Store services.
class Store(store_pb2_grpc.StoreServicer):
    def __init__(self, stop_event, product_price, account_id, bank_address):
        self.product_price = (
            product_price  # Price of the product being sold. Infinite stock.
        )
        self.account_id = account_id  # ID of the sellers account.

        # Establishing a connection with the bank server.
        self.channel = grpc.insecure_channel(bank_address)
        self.stub = bank_pb2_grpc.BankStub(self.channel)

        # Reading the balance of the seller's account.
        response = self.stub.ReadBalance(
            bank_pb2.AccountRequest(account_id=str(account_id))
        )
        self.balance = response.value

        self._stop_event = stop_event  # In order to finalize server execution.

    # Gets the price of the product.
    def GetPrice(self, request, context):
        return store_pb2.PriceResponse(value=self.product_price)

    # Processess the selling of a product by identifying an order and executing its transfer.
    def ProcessSell(self, request, context):
        try:
            response = self.stub.ExecuteTransfer(
                bank_pb2.TransferRequest(
                    order_id=int(request.order_id),
                    amount_check=int(self.product_price),
                    account_id=str(self.account_id),
                )
            )

        # If some error with the bank server occurs, returns -9.
        except:
            return store_pb2.SellResponse(value=-9)

        # Updates the seller's balance internally.
        self.balance = self.balance + self.product_price

        # Returns the status code returned by the bank server.
        return store_pb2.SellResponse(value=response.value)

    # Finalizes the execution of the server.
    def Finalize(self, request, context):
        # Closes the bank server.
        response = self.stub.Finalize(bank_pb2.FinalizeRequest())
        self.channel.close()

        # Stops the server.
        self._stop_event.set()
        return store_pb2.FinalizeResponse(bank_value=response.value, value=self.balance)


# Server's operation.
def serve():
    # Receives parameters through command line arguments.
    product_price = int(sys.argv[1])
    port = str(sys.argv[2])
    account_id = str(sys.argv[3])
    bank_address = str(sys.argv[4])

    # Instantiates the server.
    stop_event = threading.Event()
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    store_pb2_grpc.add_StoreServicer_to_server(
        Store(stop_event, product_price, account_id, bank_address), server
    )
    server.add_insecure_port("0.0.0.0:" + port)
    server.start()
    print("Server started, listening on " + port)
    stop_event.wait()
    server.stop(None)


# Entry point of the server program.
if __name__ == "__main__":
    serve()
