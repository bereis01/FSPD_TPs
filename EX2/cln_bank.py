# DCC641 - Fundamentos de Sistemas Paralelos e Distribuídos
# Segundo Exercício de Programação
# Nome: Bernardo Reis de Almeida
# Matrícula: 2021032234

import sys
import grpc
import bank_pb2
import bank_pb2_grpc


def run():
    # Retrieves command line arguments.
    account_id = str(sys.argv[1])
    server = str(sys.argv[2])

    # Connects to the server.
    channel = grpc.insecure_channel(server)
    stub = bank_pb2_grpc.BankStub(channel)

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
            case "S":
                response = stub.ReadBalance(
                    bank_pb2.AccountRequest(account_id=str(account_id))
                )
                print(response.value)
            case "O":
                _, amount = command.split()
                response = stub.RegisterOrder(
                    bank_pb2.OrderRequest(
                        account_id=str(account_id), amount=int(amount)
                    )
                )
                print(response.value)
            case "X":
                _, order_id, amount_check, dest_account_id = command.split()
                response = stub.ExecuteTransfer(
                    bank_pb2.TransferRequest(
                        order_id=int(order_id),
                        amount_check=int(amount_check),
                        account_id=str(dest_account_id),
                    )
                )
                print(response.value)
            case "F":
                response = stub.Finalize(bank_pb2.FinalizeRequest())
                print(response.value)
                break

    # Closes the channel.
    channel.close()


if __name__ == "__main__":
    run()
