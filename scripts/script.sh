NS3_DIR="/home/sabrina/ns-allinone-3.39/ns-3.39"

OUTPUT_DIR="$NS3_DIR/scratch/resultados"

NUM_SIMULATIONS=10

NODES_LIST=(5 20 80)  
MOBILITY_LIST=("RandomWalk2d" "LeaderGroup" "DynamicGroup")  
SPEED_LIST=(1 15 35)  
ROUTING_LIST=("AODV" "OLSR" "DSDV") 

for nodes in "${NODES_LIST[@]}"; do
    for mobility in "${MOBILITY_LIST[@]}"; do
        for speed in "${SPEED_LIST[@]}"; do
            for routing in "${ROUTING_LIST[@]}"; do
                for ((i=1; i<=NUM_SIMULATIONS; i++)); do
                    echo "Executando simulação: Nós=$nodes, Mobilidade=$mobility, Velocidade=$speed, Roteamento=$routing, Execução=$i"
                    
                    cd "$NS3_DIR"
                    ./ns3 run "scratch/trabalho-adrc --nNodes=$nodes --mobilityModel=$mobility --nodeSpeed=$speed --routingProtocol=$routing"
                    
                    sleep 1
                done
            done
        done
    done
done

echo "Todas as simulações foram concluídas!"