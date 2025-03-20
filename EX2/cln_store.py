# DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
# Segundo Exercício de Programação
# Nome: Bernardo Reis de Almeida
# Matrícula: 2021032234

import sys
import grpc
import bank_pb2
import store_pb2
import bank_pb2_grpc
import store_pb2_grpc


def run():
    # Retrieves command line arguments.
    account_id = str(sys.argv[1])
    bank_server = str(sys.argv[2])
    store_server = str(sys.argv[3])

    # Connects to the bank server.
    bank_channel = grpc.insecure_channel(bank_server)
    bank_stub = bank_pb2_grpc.BankStub(bank_channel)

    # Connects to the store server.
    store_channel = grpc.insecure_channel(store_server)
    store_stub = store_pb2_grpc.StoreStub(store_channel)

    # Gets and prints the product price.
    response = store_stub.GetPrice(store_pb2.PriceRequest())
    product_price = response.value
    print(product_price)

    # Reads from stdin and executes commands.
    while True:
        try:
            command = input()
        except EOFError:
            break  # Stops when reaching the end of the input.

        # Avois the split method breaking if an empty string is given.
        if not command:
            continue

        # Matches the command to an operation and executes it.
        match command.split()[0]:
            case "C":
                # Creates an order with the product price on the buyer's account.
                response = bank_stub.RegisterOrder(
                    bank_pb2.OrderRequest(
                        account_id=str(account_id), amount=int(product_price)
                    )
                )
                order_id = response.value
                print(order_id)

                # If no problems occurred, executes the sell request on the store's server.
                if order_id >= 0:
                    response = store_stub.ProcessSell(
                        store_pb2.SellRequest(order_id=order_id)
                    )
                    print(response.value)
            case "T":
                # Finalizes the execution of the store server.
                response = store_stub.Finalize(store_pb2.FinalizeRequest())
                print(response.value, response.bank_value)
                break

    # Closes the channels.
    bank_channel.close()
    store_channel.close()


if __name__ == "__main__":
    run()
