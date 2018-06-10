#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>

struct client{
    /* 2-D structure storing each connected client and it's recipients' information row by row. For ex: clients[1][0] has the first connected client and clients[1][1 to 5] have the connected client's recipients*/
    int sd;
    char username[80];
}clients[5][6];

struct recipient_names{ // stores recipient names for "to" functionality
    char n[80];
}names[5];

int main(int argc, char* argv[]){
    int num_clients;
    char buf[1024];
    char command[80];
    char username[80];
    char chat_line[80];
    int close_flag = 0;
    int max;
    char output_string[80];
    int client_count;
    int listen_sockid, new_sockid;
    struct sockaddr_in addrport;
    socklen_t addrlen;
    int opt = 1;

    //set of socket descriptors
    fd_set readfds;

    // Run as server
    if (strncmp(argv[1],"-s", sizeof("-s")-1) == 0){
        num_clients  = atoi(argv[3]) ;
        client_count = 0;
        int sd;
        int client_socket[6];
        for (int i = 0; i < 5; i++)
        {
            client_socket[i] = 0;
            clients[i][0].sd = 0;
            strcpy(clients[i][0].username,"");
        }
        client_socket[5] = fileno(stdin);
        
        if ((listen_sockid = socket(AF_INET,SOCK_STREAM,0))<0){
            perror("socket error\n");
        }
        if( setsockopt(listen_sockid, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
        {
            perror("setsockopt");
        }
        
        int status = fcntl(listen_sockid, F_SETFL, fcntl(listen_sockid, F_GETFL, 0) | O_NONBLOCK);
        
        if (status == -1){
            perror("fcntl error");
        }

        addrport.sin_family = AF_INET;
        addrport.sin_port = htons(atoi(argv[2]));
        addrport.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(addrport.sin_zero, '\0', sizeof(addrport.sin_zero));
        addrlen = sizeof(addrport);

        
        if (bind(listen_sockid,(struct sockaddr*)&addrport,sizeof addrport)<0){
            perror("bind error\n");
        }
        
        if(listen(listen_sockid,5)<0){
            perror("listen error\n");
        }

        printf("Chat server begins [port= %s] [nclient= %s]\n", argv[2], argv[3]);
        
        while(true){
            printf("\na3chat_server: ");
            //reset the readfds set
            fflush(stdout);
            max =0;
            FD_ZERO(&readfds);
            
            FD_SET( listen_sockid , &readfds);
            max = listen_sockid;
            
            for (int i = 0 ; i < 5 ; i++){
                //socket descriptor
                sd = client_socket[i];
                //if valid socket descriptor then add to read list
                if(sd > 0)
                FD_SET( sd , &readfds);
                //highest file descriptor number, need it for the select function
                if(sd > max)
                max = sd;
            }
            FD_SET( client_socket[5] , &readfds); //adding stdin
            
            //wait till activity
            int activity = select( max+1 , &readfds , NULL , NULL , NULL);
            
            if (activity < 0){
                perror("select error\n");
            }
            
            // For connection requests
            if (FD_ISSET(listen_sockid, &readfds)){
                if ((new_sockid = accept(listen_sockid, (struct sockaddr *)&addrport, (socklen_t*)&addrlen))<0){
                    perror("accept error\n");
                }
                
                //add new socket to array of sockets
                for (int i = 0; i < 5; i++){
                    //if position is empty
                    if( client_socket[i] == 0 ){
                        client_socket[i] = new_sockid;
                        break;
                    }
                }
            }
            
            //IO operation on socket or stdin
            for (int i = 0; i < 6; i++)
            {
                sd = client_socket[i];
                
                if (FD_ISSET( sd, &readfds))
                {
                    memset(buf,0,sizeof(buf));
                    while (read(sd, buf, sizeof(buf))>0) {
                        // execute the command and send response
                        if (strncmp(buf,"open", sizeof("open")-1) == 0){
                            strcpy(username,buf+(sizeof("open ")-1));
                            username[strlen(username) - 1] = 0;
                            int error = 0;
                            
                            //Client limit reached
                            if(client_count == num_clients){
                                send(sd, "[server] error: Reached client limit",sizeof("[server] error: Reached client limit"),0);
                                break;
                            }
                            
                            for(int index=0; index<5; index++){
                                //Username taken
                                if (strncmp(clients[index][0].username,username, sizeof(username)) == 0){
                                    send(sd, "[server] error: Username taken",sizeof("[server] error: Username taken"),0);
                                    error = 1;
                                    break;
                                }
                            }
                            
                            
                            if (error == 0){
                                int flag = 0;
                                for(int ind=0; ind<5; ind++){
                                    //Client has a session already
                                    if ((clients[ind][0].sd == sd) && (strcmp(clients[ind][0].username,"")!=0)){
                                        send(sd, "[server] error: client has session already",sizeof("[server] error: client has session already"),0);
                                        flag = 1;
                                        break;
                                    }
                                }
                                if (flag == 0){
                                    for(int ind = 0; ind<5;ind++){
                                        if (clients[ind][0].sd == 0){
                                            strcpy(output_string,"[server] User connected: ");
                                            strcat(output_string,username);
                                            strcpy(clients[ind][0].username,username);
                                            clients[ind][0].sd = sd;
                                            client_count++;
                                            if(send(sd,output_string,sizeof(output_string),0)<0){
                                                perror("send unsuccessful\n");
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        
                        if (strncmp(buf,"who", sizeof("who")-1) == 0){
                            char users_string[80];
                            strcpy(users_string,"[server]: Current users: ");
                            for ( int j =0; j<5; j++){
                                if (strcmp(clients[j][0].username,"")!=0){
                                    strcat(users_string,clients[j][0].username);
                                    strcat(users_string, " ");
                                }
                            }
                            if(send(sd,users_string,sizeof(users_string),0)<0){
                                perror("send error\n");
                            }
                        }
                        // Exit from server stdin
                        if (strncmp(buf,"exit", sizeof("exit")-1) == 0){
                            for(int l=0; l<6; l++){
                                close(client_socket[l]);
                                client_socket[l] = 0;
                            }
                            close(listen_sockid);
                            return 0;
                        }
                        
                        if (strncmp(buf,"client_exiting", sizeof("client_exiting")-1) == 0){
                            //remove the username from clients structure
                            for(int m=0; m<5; m++){
                                if(clients[m][0].sd == sd){
                                    strcpy(clients[m][0].username,"");
                                    client_count = client_count-1;
                                    for (int j =1; j <6; j++){
                                        // clear recipient info in struct
                                        clients[m][j].sd = 0;
                                        strcpy(clients[m][j].username,"");
                                    }
                                    close(client_socket[m]);
                                    client_socket[m] = 0;
                                    clients[m][0].sd =0;
                                    break;
                                }
                            }
                        }
                        if (strncmp(buf,"close", sizeof("close")-1) == 0){
                            // remove the username from clients struct
                            printf("close recieved");
                            for(int n=0;n<5;n++){
                                if(clients[n][0].sd == sd){
                                    strcpy(clients[n][0].username,"");
                                    client_count = client_count-1;
                                    for (int j =1; j <6; j++){
                                        // clear recipient info in struct
                                        clients[n][j].sd = 0;
                                        strcpy(clients[n][j].username,"");
                                    }
                                    clients[n][0].sd =0;
                                    break;
                                }
                            }
                            printf("sending\n");
                            if(send(sd,"[server] done",sizeof("[server] done"),0)<0){
                                perror("\n Send error\n");
                            }
                        }
                        if (strncmp(buf,"to ", sizeof("to ")-1) == 0){
                            char recipient_string[1024];
                            strcpy(recipient_string,buf+(sizeof("to ")-1));
                            int connected =0;
                            int connected_sd = 0;
                            
                            for(int i = 0; i< 5; i++){ // Find this sender's info in the clients struct
                                if(clients[i][0].sd == sd){ // sender info found
                                    sscanf( recipient_string, "%s %s %s %s %s", names[0].n, names[1].n, names[2].n, names[3].n, names[4].n );
                                    for(int k=0;k<5;k++){ //go through all the recipients
                                        if(strcmp(names[k].n,"")!=0){
                                            for (int ind =0; ind <5; ind++){ //find if recipient connected
                                                if(strcmp(clients[ind][0].username,names[k].n)==0){
                                                    connected = 1;
                                                    connected_sd = clients[ind][0].sd;
                                                    break;
                                                }
                                            }
                                            if(connected==1){
                                                for (int j =1; j <6; j++){
                                                    if(clients[i][j].sd == 0){ // if recipient has a slot empty
                                                        // fill recipient info in struct
                                                        clients[i][j].sd = connected_sd;
                                                        strcpy(clients[i][j].username,names[k].n);
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (strncmp(buf,"<", sizeof("<")-1) == 0){

                            for(int i = 0; i< 5; i++){ // Find this sender's info in the clients struct
                                if(clients[i][0].sd == sd){ // sender info found
                                    // Form the chat line withs ender username
                                    strcpy(chat_line,"[");
                                    strcat(chat_line, clients[i][0].username);
                                    strcat(chat_line,"]");
                                    strcat(chat_line,buf+(sizeof("<")-1));
                                    chat_line[strlen(chat_line) - 1] = 0;
                                    for(int j=1; j< 6; j++){ //circle through all its recipients
                                        if(clients[i][j].sd!=0){
                                            if(send(clients[i][j].sd,chat_line,sizeof(chat_line),0)<0){
                                                perror("send error\n");
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Run as client
    else if (strncmp(argv[1],"-c", sizeof("-c")-1) == 0){
        int polling_fds[2];
        int sd;
        struct hostent	*hp;
        
        hp= gethostbyname(argv[3]);
        
        if (hp == (struct hostent *) NULL) {
            perror("\ngethostbyname error\n");
        }
        memset ((char *) &addrport, 0, sizeof addrport);
        memcpy ((char *) &addrport.sin_addr, hp->h_addr, hp->h_length);
        addrport.sin_family= hp->h_addrtype;
        addrport.sin_port= htons(atoi(argv[2]));
        
        printf("\nChat client begins (server: %s, port %s)\n",argv[3],argv[2]);
        
        sd= socket(hp->h_addrtype, SOCK_STREAM, 0);
        if (sd < 0) {
            perror("\nsocket error\n");
        }
        
        // Enabling keepalive on the socket with default values
        int val = 1;
        setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof val);

        
        while(true){
            printf("\na3rchat_client: ");
            fflush(stdout);
            
            // get open command
            fgets(command, sizeof(command), stdin);
            if (strncmp(command,"open", sizeof("open")-1) == 0){
                if (connect(sd, (struct sockaddr *) &addrport, sizeof addrport) < 0) {
                    perror("\nConnect error\n");
                }
                // Send connection request to server by writing to infifo
                if (send(sd, command, strlen(command),0)==-1) {
                    perror("send error\n");
                }
            }
            while(true){
                //poll stdin and socket
                FD_ZERO(&readfds);
                polling_fds[0] = sd;
                polling_fds[1] = fileno(stdin);
                FD_SET( sd, &readfds);
                FD_SET( fileno(stdin) , &readfds);
                
                //wait till activity
                int activity = select( sd+1 , &readfds , NULL , NULL , NULL);
                
                //circle through the readfds set and read
                for (int i=0; i< 2; i++){
                    if (FD_ISSET(polling_fds[i], &readfds)){
                        memset(buf,0,sizeof(buf));
                        while (read(polling_fds[i], buf, sizeof(buf))>0) {
                            
                            if (strncmp(buf,"exit", sizeof("exit")-1) == 0){
                                send(sd, "client_exiting", sizeof("client_exiting"),0);
                                close(sd);
                                return 0;
                            }

                            else if((strncmp(buf,"to", sizeof("to")-1) == 0) || (strncmp(buf,"who", sizeof("who")-1) == 0) || (strncmp(buf,"open", sizeof("open")-1) == 0) || (strncmp(buf,"<", sizeof("<")-1) == 0)){
                                if(send(sd, buf, sizeof(buf),0)<0){
                                    perror("\nWrite error\n");
                                }
                                break;
                            }
                            else if((strncmp(buf,"close", sizeof("close")-1) == 0)){
                                if(send(sd, buf, sizeof(buf),0)<0){
                                    perror("\nWrite error\n");
                                }
                                break;
                            }
                            else if (strncmp(buf,"[server] done", sizeof("[server] done")-1) == 0){
                                close_flag = 1;
                                break;
                            }

                            else if(strncmp(buf,"[", sizeof("[")-1) == 0){
                                printf("\n%s\n",buf);
                                fflush(stdout);
                                break;
                            }
                            else{
                                printf("\nInvalid command\n");
                                fflush(stdout);
                                break;
                            }
                                    
                        }
                        if (close_flag == 1){
                            break;
                        }
                    }
                }
                if (close_flag == 1){
                    close(sd);
                    close_flag = 0;
                    break;
                }
            }
        }
    }
}
                                       


