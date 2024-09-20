# ver-q2

To build docker image:
`sudo docker build -t verq2 .`

To run docker image:
`sudo docker run -v "$(pwd):/home/verq2/" -it verq2`

This code is not cleaned up yet, may contain pieces of unused code (ex. model-fix and solveinparts).

Barvinok has been removed from docker image due to broken links.

To make code:
`make`

Instructions can be found for running:
`./main -h`

(or alternately `./provero_impl -h` for our implementation of the Provero algorithm)

Networks follow the nnet layout: https://github.com/sisl/NNet, but without items 5--8.
