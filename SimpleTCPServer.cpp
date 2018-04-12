#include "commonutils.h"
#include <vector>

using namespace std;

int sendData(SOCKET socket, const char* data){
    int sendResult = 0;

    sendResult = send(socket, data, strlen(data), 0);
    if (sendResult == SOCKET_ERROR) {
        cout << "send failed with error: " << WSAGetLastError() << endl;
        closesocket(socket);
        WSACleanup();
        return 1;
    }
    cout << "Send message: " << data;
    cout << "(" << strlen(data) << " b)" << endl;
}

void getValues(SOCKET socket, float& maxEl, float& minEl, float& Avg){
    sendData(socket, "OK");
    char arr[512];
    recv(socket, arr, 512, 0);
    cout << "Received message: " << arr;
    cout << "(" << strlen(arr) << " b)" << endl;
    string str(arr);
    vector<float> vect;
    int pos = 0;
    string token;
    while ((pos = str.find(' ')) != string::npos) {
        token = str.substr(0, pos);
        vect.push_back(atof(token.c_str()));
        str.erase(0, pos + 1);
    }
    minEl = vect[0];
    maxEl = vect[0];
    float Sum = 0;
    for (unsigned i = 0; i < vect.size(); i++){
        minEl = (vect[i] < minEl) ? vect[i] : minEl;
        maxEl = (vect[i] > maxEl) ? vect[i] : maxEl;
        Sum += vect[i];
    }
    Avg = Sum / (float)vect.size();
    sendData(socket, "OK");
    cout << endl;
}

void printFloat(SOCKET socket, float value){
    char buffer[50];
    sprintf(buffer, "%.2f", value);
    sendData(socket, buffer);
    cout << endl;
}

void printError(SOCKET socket){
    sendData(socket, "Unknown command");
}

int main(int argc,char* argv[]) {
	//Буфер повідомлень
	char dataBuffer[BUFFER_SIZE];
	SOCKET serverSocket;
	SOCKET clientSocket;
	float maxElem, minElem, avg;

	struct sockaddr_in serverAddr;
	struct sockaddr_in clientAddr;

#ifdef OS_WINDOWS
	int clientAddrLen = sizeof(clientAddr);
#else
	unsigned int clientAddrLen = sizeof(clientAddr);
#endif
	int nPort = 5150;

	char* portPrefix = "-port:";
	char strPort[6];
    //Отримуємо порт серверу з командного рядка

	if (getParameter(argv,argc,"-port",strPort,':')) {
		int tempPort = atoi(strPort);
		if (tempPort>0) nPort = tempPort;

		else {
			cout<<"\nError command argument "<<argv[0]<<" -port:<integer value>\n";
			cout<<"\nUsage "<<argv[0]<<" -port:<integer value>\n";
		}
	}

	memset(&serverAddr,0,sizeof(serverAddr));

	memset(&clientAddr,0,sizeof(clientAddr));
	memset(dataBuffer,0,BUFFER_SIZE);
	if (initSocketAPI()) {
		socketError(true,"init socket API");
	}

	//Створюємо слухаючий сокет
	if ((serverSocket=socket(AF_INET,SOCK_STREAM, IPPROTO_TCP))<=0) {
		return socketError(true,"create socket",true);
	}

	//Заповнюємо структуру serverAddr для зв’язування
	//функцією bind
	serverAddr.sin_family = AF_INET;
	//Переходимо до мережевого порядку байт
	serverAddr.sin_port = htons(nPort);
	//Слухаємо всі інтерфейси

	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(serverSocket,(struct sockaddr*)&serverAddr, sizeof(serverAddr))!=0) {
		closeSocket(serverSocket);
		return socketError(true,"bind socket",true);
	}

	//Переводимо сокет в пасивний режим прослуховування
	//вхідних запитів
	if (listen(serverSocket,LINEQ)!=0) {
		closeSocket(serverSocket);
		return socketError(true,"listen socket",true);
	}

	//Цикл прийому вхідних запитів
	cout<<"Server start at port: "<<nPort<<endl;
	do {
		if ((clientSocket=accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen))<0) {
			closeSocket(serverSocket);
			return socketError(true,"accept socket",true);
		}

		printInfo(argv[0]);
		cout<<"Connected with: ";
		cout<<inet_ntoa(clientAddr.sin_addr)<<":";
		cout<<ntohs(clientAddr.sin_port)<<endl;

		//Цикл прийому вхідних запитів
		int nRec;
		do {
			nRec = recv(clientSocket, dataBuffer, BUFFER_SIZE,0);
			dataBuffer[nRec]='\0';
			//Відображаємо прийняті дані
			if (stricmp(dataBuffer, disconnetClientCmd) && nRec > 0) {
				cout << "Received message: " << dataBuffer;
				cout << "(" << nRec << " b)" << endl;
				if(strcmp(dataBuffer, "Array") == 0)
                    getValues(clientSocket, maxElem, minElem, avg);
                else if(strcmp(dataBuffer, "getMax") == 0)
                    printFloat(clientSocket, maxElem);
                else if(strcmp(dataBuffer, "getMin") == 0)
                    printFloat(clientSocket, minElem);
                else if(strcmp(dataBuffer, "getAvg") == 0)
                    printFloat(clientSocket, avg);
                else
                    printError(clientSocket);
			} else {
				cout<<"Disconnect client: ";
				cout<<inet_ntoa(clientAddr.sin_addr)<<":";
				cout<<ntohs(clientAddr.sin_port)<<endl;
				break;
			}
		} while (1);
		//Закриваємо клієнтський сокет
		closeSocket(clientSocket);
	} while (1);

	//Закриваємо сокети і звільняємо системні ресурси
	closeSocket(serverSocket);
	deinitSocketAPI();
	return 0;
}
