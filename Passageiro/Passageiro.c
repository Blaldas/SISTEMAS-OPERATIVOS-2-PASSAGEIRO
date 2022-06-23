#define _CRT_SECURE_NO_WARNINGS	//TOU FARTO DESTA PORCARIA

#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TAM 200

#define PIPE_PASSAGEIRO_2_CONTROL TEXT("\\\\.\\pipe\\PIPE_PASSAGEIRO_2_CONTROL")
#define PIPE_CONTROL_2_PASSAGEIRO TEXT("\\\\.\\pipe\\PIPE_CONTROL_2_PASSAGEIRO")


//estados passageiro
#define ESTADO_CONECTAR_PASSAGEIRO 1						
#define ESTADO_ARGUMENTOS_INVALIDOS 2							
#define ESTADO_ESPERA_POR_AVIAO 3
#define ESTADO_EMBARCOU_PASSAGEIRO 4
#define ESTADO_PASSAGEIRO_VOO 5
#define ESTADO_PASSAGEIRO_TIMEOUT 6
#define ESTADO_PASSAGEIRO_CHEGOU_DESTINO 7
#define ESTADO_PASSAGEIRO_QUER_DESLIGAR 8
#define ESTADO_CONTROLO_DESLIGA_PASSAGEIROS 9 			



typedef struct {
	TCHAR nome[TAM];
	int x, y;
	int estado;
	TCHAR aeroportoInicial[TAM];
	TCHAR aeroportoDestino[TAM];
	//int tempoEspera;				//se <0, Não partilhar, não é necessário, fica apenas no programa passageiro
}passageiro, pPassageiro;

DWORD WINAPI leNamedPipes(LPVOID lpParam);
DWORD WINAPI leInputUser(LPVOID lpParam);

void enviaMensagemPipe(passageiro passag);

HANDLE hPipePassageiro2Control;
HANDLE hPipeControl2Passageiro;
passageiro passag;





int _tmain(int argc, LPTSTR argv[]) {

	int tempoEspera = -1;	
	boolean* estadoDesligar = FALSE;



#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
/*
	//passageiro.exe 
	//verifica argumentos.
	if (argc != 5 && argc != 4) {
		_tprintf(TEXT("\nPassageiro.exe <nome passageiro> <Aeroporto inicial> <aeroporto destino> <tempo de espera>...\nA sair."));
		exit(1);
	}
	//tempo de espera, se existir
	if (argc == 5) {
		tempoEspera = _wtoi(argv[4]) * 1000;	//passa de segundos para milisegundos
	}

*/
	switch (argc) {
	case 4:
		break;
	case 5:
		tempoEspera = _wtoi(argv[4]) * 1000;	//passa de segundos para milisegundos
		break;
	default:
		_tprintf(TEXT("\nPassageiro.exe <nome passageiro> <Aeroporto inicial> <aeroporto destino> <tempo de espera>...\nA sair."));
		exit(1);
	}








	//copia nomes dos aeroportos
	wcscpy(passag.nome, argv[1]);
	wcscpy(passag.aeroportoInicial, argv[2]);
	wcscpy(passag.aeroportoDestino, argv[3]);

	//coloca estado inicial
	passag.estado = ESTADO_CONECTAR_PASSAGEIRO;

	//CONTROL 2 PASSAGEIRO
	if (!WaitNamedPipe(PIPE_CONTROL_2_PASSAGEIRO, NMPWAIT_WAIT_FOREVER)) {								//espera que o pipe, já criado, tenha espaço para aceitar a coneção
		_tprintf(TEXT("Ligar ao pipe '%s'! (WaitNamedPipe) -> %d\n"), PIPE_CONTROL_2_PASSAGEIRO, GetLastError());
		exit(-1);
	}

	_tprintf(TEXT("A ligar ao pipe do control\n"));
	hPipeControl2Passageiro = CreateFile(PIPE_CONTROL_2_PASSAGEIRO, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);					//Abre o pipe com o CreateFile
	if (hPipeControl2Passageiro == NULL) {
		_tprintf(TEXT("Erro a ligar ao pipe '%s'! (CreateFile) -> %d\n"), PIPE_CONTROL_2_PASSAGEIRO, GetLastError());
		exit(-1);
	}

	
/*	if (tempoEspera != -1)
		if (SetNamedPipeHandleState(hPipeControl2Passageiro, NULL, NULL, tempoEspera) != 0){
			_tprintf(TEXT("\nErro a colocar timeout... A sair"));
			exit(1);
		}
*/
	//PASSAGEIRO 2 CONTROL
	if (!WaitNamedPipe(PIPE_PASSAGEIRO_2_CONTROL, NMPWAIT_WAIT_FOREVER)) {								//espera que o pipe, já criado, tenha espaço para aceitar a coneção
		_tprintf(TEXT("Ligar ao pipe '%s'! (WaitNamedPipe) -> %d\n"), PIPE_PASSAGEIRO_2_CONTROL, GetLastError());
		exit(-1);
	}

	_tprintf(TEXT("A ligar ao pipe do control\n"));
	hPipePassageiro2Control = CreateFile(PIPE_PASSAGEIRO_2_CONTROL, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);					//Abre o pipe com o CreateFile
	if (hPipePassageiro2Control == NULL) {
		_tprintf(TEXT("Erro a  ligar ao pipe '%s'! (CreateFile) -> %d\n"), PIPE_PASSAGEIRO_2_CONTROL, GetLastError());
		exit(-1);
	}

	_tprintf(TEXT("Conecção com os pipes completa\n"));

	//envia mensagem incial de coneção
	enviaMensagemPipe(passag);


	//lança a thread para receber dados do pipe
	//LANCA THREAD PARA ESCREVER O "SAIR"
	//faz WaitForMultipleObject(as 2s threads) com opção de esperar apenas por uma delas
	//faz close da outra thread ->TerminateThread() <- da thread que restar
	HANDLE hThread[2];

	hThread[0] = CreateThread(NULL, 0, leNamedPipes, &estadoDesligar, 0, NULL);		// 0 -> thread de leitura.
	hThread[1] = CreateThread(NULL, 0, leInputUser, &estadoDesligar, 0, NULL);		// 1 -> thread de escrita.
	
	DWORD r = WaitForMultipleObjects(2, &hThread, FALSE, INFINITE);
	//mata a outra thread
	if (r - 0 == 0) {		//thread de leitura desligou-se
		//ExitThread(hThread[1]);
		TerminateThread(hThread[1], 0);

	}
	else {
		//ExitThread(hThread[0]);
		TerminateThread(hThread[0], 0);

	}

	//fecha handles e afins
	//threads
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	//pipes
	CloseHandle(hPipeControl2Passageiro);
	CloseHandle(hPipePassageiro2Control);

	return 0;
}


DWORD WINAPI leInputUser(LPVOID lpParam) {
	boolean* estadoDesligar = (boolean*)lpParam;
	//fica a espera de um possivel input do user para cancelar
	_tprintf(TEXT("\nPrima ENTER para desligar programa, interrompendo tudo."));
	_gettchar();

	//modifica variavel
	passag.estado = ESTADO_PASSAGEIRO_QUER_DESLIGAR;
	
	//envia para o pipe com o novo estado
	estadoDesligar = TRUE;
	enviaMensagemPipe(passag);


}

void enviaMensagemPipe(passageiro passag) {
	DWORD n;
	if (!WriteFile(hPipePassageiro2Control, &passag, sizeof(passageiro), &n, NULL)) {			//escreve a frase no pipe
		_tprintf(TEXT("Erro a enviar mensagem de argumentos inválidos para o pipe -> %d\n"), GetLastError());
		exit(-1);
	}
}

DWORD WINAPI leNamedPipes(LPVOID lpParam) {
	//HANDLE hPipeLeitura = (HANDLE) lpParam;
	boolean* estadoDesligar = (boolean*)lpParam;
	LPDWORD n;


	int msg = 0;
	while (1) {
	
		BOOL ret = ReadFile(hPipeControl2Passageiro, &passag, sizeof(passag), &n, NULL);								//lê o pipe usando "ReadFile()"
		if (!ret || !n) {
			if(!estadoDesligar)
				_tprintf(TEXT("Erro a ler do pipe -> %d\n"), GetLastError());
			break;
		}

		/*
			//estados passageiro										quem envia		/		quem recebe
			#define ESTADO_CONECTAR_PASSAGEIRO 1						passageiro				controlo
			#define ESTADO_ARGUMENTOS_INVALIDOS 2						controlo				passageiro
			#define ESTADO_ESPERA_POR_AVIAO 3							controlo				passageiro
			#define ESTADO_EMBARCOU_PASSAGEIRO 4						controlo				passageiro
			#define ESTADO_PASSAGEIRO_VOO 5								controlo				passageiro
			#define ESTADO_PASSAGEIRO_TIMEOUT 6							passageiro				controlo
			#define ESTADO_PASSAGEIRO_CHEGOU_DESTINO 7					passageiro e controlo	passageiro e controlo
			#define ESTADO_PASSAGEIRO_QUER_DESLIGAR 8					passageiro				controlo
			#define ESTADO_CONTROLO_DESLIGA_PASSAGEIROS 9				controlo				passageiro e controlo

			Controlador:
			O controlador cria os pipes e fica à espera que alguem se conecte aos pipes
			Quando alguem se conecta cria uma thread que trata de ler as coisas.
			No inicio, essa thread espera uma mensagem com o ESTADO_CONECTAR_PASSAGEIROS, se as coisas estiverem mal deevolve ESTADO_ARGUMENTOS_INVALIDOS.
			Se estiver tudo bem, adiciona à lista o avião com ambas as HANDLES para os pipes e responde com ESTADO_ESPERA_POR_AVIAO. Este é o único momento em que esta thread escreve o que quer que seja.
			Quem trata de escrever é a thread geral que também fala com os aviões!
			Esta thread limita-se a ler as mensagens: ESTADO_PASSAGEIRO_TIMEOUT e ESTADO_PASSAGEIRO_QUER_DESLIGAR, reagindo às mesmas.
			Quando um aviao chega ao destino com os passageiros, a thread geral de comnunicação envia para o programa passageiro ESTADO_PASSAGEIRO_CHEGOU_DESTINO. O programa passageiro, ao receber esta mensagem devolve uma mensagem com o mesmo estado!

			Passageiro:
			O passageiro tem duas threads distintas da main.
			Uma das threads fica à espera de um input do user para desligar o programa. Quando o faz, envia para o controlador o estado ESTADO_PASSAGEIRO_QUER_DESLIGAR.
			A outra thread é de leitura e fica a ler as mensagens do controlador. As mensagens podem ser de qualquer tipo acima ligado e deverá reagir de acordo.
			Reações da thread de leitura:
						ESTADO_ARGUMENTOS_INVALIDOS -> indica que existem erros no nome das cidades colocadas e sai
						ESTADO_ESPERA_POR_AVIAO -> indica que o passageiro encontra-se à espera de uma avião
						ESTADO_EMBARCOU_PASSAGEIRO -> indica que o passageiro foi embarcado num avião
						ESTADO_PASSAGEIRO_VOO -> indica que passageiro está em voo. Apresenta as suas coordenadas
						Quando existe um timeout de espera pelo avião envia: ESTADO_PASSAGEIRO_TIMEOUT
						ESTADO_PASSAGEIRO_CHEGOU_DESTINO -> indica que chegou ao destino. Devolve o passageiro recebido para o controlo poder desligar a thread. retorna à main
						ESTADO_CONTROLO_DESLIGA_PASSAGEIROS -> indica que o controlo desligou tudo. Desvolve o passageiro recebido para o controlo poder desligar a thread
			A main do programa passageiro encontra-se a fazer WaitForMultipleObject, sendo que assim que desbloqueia uma mata a outra e desliga-se


			o passageiro conetca-se com o estado ESTADO_CONECTAR_PASSAGEIRO, o seu nome, o aeroporto inicial e o aeroporto de destino
			o controlador recebe o estado ESTADO_CONECTAR_PASSAGEIRO, verifica o se os aeroportos existem.
			Se não existirem, devolve ESTADO_ARGUMENTOS_INVALIDOS e desliga a thread onde troca mensagens.
				O passageiro lê ESTADO_ARGUMENTOS_INVALIDOS, mostra no ecra o erro, e acaba a thread de leitura.
			Se tudo estiver bem, o controlador coloca as coordenadas no passageiro, coloca-o na lista de passageiros e devolve ESTADO_ESPERA_POR_AVIAO.
			Quando o passageiro recebe ESTADO_ESPERA_POR_AVIAO, um timeout é activado.
			Se o timeout chegar ao fim -> ainda a ver como fazer <- então o passageiro escreve ESTADO_PASSAGEIRO_TIMEOUT e procede a sair da thread de onde enviou essa mensagem, desligando o programa.
			Se não existiu timeout, entao o passageiro foi colocado num avião. Quando o passageiro é colocado num avião é enviado ESTADO_EMBARCOU_PASSAGEIRO, o programa passageiro deverá indicar isso
				Note-se que, a partir daqui, o timeout poderá ser desligado (ou mantido, idk....)
			A posição deverá ser atualizada a cada segundo, recebendo ESTADO_PASSAGEIRO_VOO e mostrando no ecra as coordenadas.
			Quando o controlador percebe que o avião chega ao destino, desembarca todas as pessoas, pelo que envia aos passageiros o estado ESTADO_PASSAGEIRO_CHEGOU_DESTINO.
			Quando o passageiro recebe o estado ESTADO_PASSAGEIRO_CHEGOU_DESTINO, compreende que chegou ao fim. Desliga a thread de leitura e, poesteiorimente, o programa.
			A qualquer momento o uitlizador do passageiro pode querer sair. Para isso escreve "sair". A thread com o utilizador percebe isso e envia o estado ESTADO_PASSAGEIRO_QUER_DESLIGAR, dendo que, depois desliga-se
				O fecho dessa thread acaba por deligar o programa -> WaitForMultipleObjects().
			Quando o controlador recebe ESTADO_PASSAGEIRO_QUER_DESLIGAR remove o passageiro da lista de passageiros e desliga a thread relacionada com o mesmo (desligando o pipe idk)
		*/
		if (passag.estado == ESTADO_ARGUMENTOS_INVALIDOS) {
			_tprintf(TEXT("\nOs nomes dos aeroportos foram mal colocados.\nA sair"));
			break;
		}
		else if (passag.estado == ESTADO_ESPERA_POR_AVIAO) {
			_tprintf(TEXT("\nA espera para entrar num avião!"));
		}
		else if (passag.estado == ESTADO_EMBARCOU_PASSAGEIRO) {
			_tprintf(TEXT("\nFoi embarcado num avião"));
		}
		else if (passag.estado == ESTADO_PASSAGEIRO_VOO) {
			_tprintf(TEXT("\nA vooar!\t{ %d ; %d }"), passag.x, passag.y);
		}
		else if (passag.estado == ESTADO_PASSAGEIRO_CHEGOU_DESTINO) {
			_tprintf(TEXT("\nCHEGOU AO SEU DESTINO!!\nA sair"));
			enviaMensagemPipe(passag);
			break;
		}
		else if (passag.estado == ESTADO_CONTROLO_DESLIGA_PASSAGEIROS) {
			_tprintf(TEXT("\nControlo decidiu desligar!!\nA sair..."));
			enviaMensagemPipe(passag);
			break;
		}

		fflush(stdout);
	}
}
