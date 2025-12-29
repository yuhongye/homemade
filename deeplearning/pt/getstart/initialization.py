import torch

data = [[1, 2],[3, 4]]
x_data = torch.tensor(data)
print(x_data)
print(type(x_data))
print("----------------------------")

x_ones = torch.ones_like(x_data)
print(f"Ones Tensor: \n {x_ones} \n")

x_rand = torch.rand_like(x_data, dtype=torch.float)
print(f"Random Tensor: \n {x_rand} \n")
print("----------------------------")

shape = (2, 3, )
rand_tensor = torch.rand(shape)
ones_tensor = torch.ones(shape)
zeros_tensor = torch.zeros(shape)
print(f"Random Tensor: \n{rand_tensor}\n")
print(f"Ones Tensor: \n{ones_tensor}\n")
print(f"Zeros Tensor: \n{zeros_tensor}\n")

print("----------------------------")
tensor = torch.rand(3, 4)
print(f"Shape of tensor: {tensor.shape}")
print(f"Datatype of tensor: {tensor.dtype}")
print(f"Device tensor is stored on: {tensor.device}")

print("----------------------------")
tensor = torch.ones(4, 4)
tensor[:,1] = 0
print(tensor)

t1 = torch.cat([tensor, tensor, tensor], dim=1)
print(t1)

print(f"tensor.mul(tensor) \n {tensor.mul(tensor)} \n")
print(f"tensor.T: {tensor.T}")
print(f"tensor @ tensor.T \n {tensor @ tensor.T}")

print(tensor, "\n")
tensor.add_(5)
print(tensor)



