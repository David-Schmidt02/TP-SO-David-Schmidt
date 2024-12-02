#!/bin/sh

#README
#Versión: 1.0
#Estado actual: Primera versión Funcional. Puede hacer make, clear y correr los programas.
#		No verifica errores. Porfavor no intentar algo en el orden equivocado. Muchas gracias

#Proximas actualizaciones:
#	1. Agregar un loop para make y clean-----DONE-------
#	2. Agregado de parametros (Inicialmente para help)-----DONE------
#	3. REVISAR LAS EJECUCIONES. Es posible que no corran correctamente.------DONE-------

#Funciones

#Muestra en pantalla información sobre el script
help(){
	cat << _EOF_
	Este script tiene la función de ser un menú interactivo para facilitar el envio de comandos.

	Versión 1.0
	¿Como utilizarlo?
	Es un archivo ejectable. Con tirar el siguiente comando "./TPCtrl.sh" estando en la carpeta correspondiente va a correr.
	De esta forma iniciarás el Menú Principal
	Puedes agregarle los siguentes parametros para pasar a un modo diferente (EJ: ./TPCtrl.sh help)


	PARAMETROS
	help
		Muestra información sobre como usar el script

	consola
		Permite realizar el make y el make clean SOLO para consola
	
	kernel
		Permite realizar el make y el make clean SOLO para kernel

	cpu
		Permite realizar el make y el make clean SOLO para cpu

	filesystem
		Permite realizar el make y el make clean SOLO para filesystem

	memoria
		Permite realizar el make y el make clean SOLO para memoria

	Ante la necesidad de más comandos en el menú, o agregar más parametros avisar para actualizar.
	Muchas gracias
	Maxikiosco
_EOF_
}

##------------------------------------ENTREGA FINAL-----------------------------------------------------------------------

#Entrega Final:
EntregaFinal(){
	clear
	cd ~/

		echo "\n----------Creando-Carpetas-------------\n"
	mkdir ~/fs
	touch ~/fs/sbloque.dat
	mkdir ~/fs/fcb
	path_fs=$(pwd)

	echo "\n------ Instalar commons y clonar el repo? -----(y/n)\n"
	read rts;
	if [ "$rts" = "y" ]; then
			echo "\n----------Get commons------------------\n"
		
			git clone https://github.com/sisoputnfrba/so-commons-library.git
			cd so-commons-library
			make install
			cd ~/

			echo "\n----------Get Pruebas------------------\n"
			git clone https://github.com/sisoputnfrba/tuki-pruebas
	
			echo "\n----------Get TP-----------------------\n"
			git clone https://github.com/sisoputnfrba/tp-2023-1c-maxikiosco

			echo "\n---------Install Gnome-Terminal----------\n"
			sudo apt install gnome-terminal


	fi

			echo "\n----------Fin de ejecución Entrega Final-----------------------\n\n\n"

}


##------------------------------------CambiarConfig-----------------------------------------------------------------------

CambiarConfig(){
	cd ~/
	path_fs=$(pwd)

	cd ~/tp-2023-1c-maxikiosco
	echo "\n Cambiando configuraciones: ¿Para cual prueba?\n"
	while true; do
		echo "1)Config_Base\n 2)Config_Deadlock\n 3)Config_Memoria\n 4)Config_File_System\n 5)Config_Errores\n "
		read choice;
			case $choice in
				1) Config_Base;;
				2) Config_Deadlock;;
				3) Config_Memoria;;
				4) Config_File_System;;
				5) Config_Errores;;
			esac
		break
	done
	
	cd ../IPs/
	clear
	make
	./IP.exe $path_fs
	clear
	echo "Actualización de configuraciones completada"

}

Config_Base(){
	cd kernel
	echo pwd
	echo "\n 1)FIFO 2)HRRN \n"
	read choice;
	if [ "$choice" = "1" ]; then
		cp ../IPs/configuraciones/base/kernel_base_FIFO.config kernel.config	
	else 
		cp ../IPs/configuraciones/base/kernel_base_HRRN.config kernel.config	
	fi


	cp ../IPs/configuraciones/base/kernel_base.config kernel.config	
	cd ../memoria
	cp ../IPs/configuraciones/base/memoria_base.config memoria.config	
	cd ../cpu
	cp ../IPs/configuraciones/base/cpu_base.config cpu.config
	cd ../filesystem
	cp ../IPs/configuraciones/base/filesystem_base.config filesystem.config

}

Config_Deadlock(){
	cd kernel
	cp ../IPs/configuraciones/deadlock/kernel_deadlock.config kernel.config	
	cd ../memoria
	cp ../IPs/configuraciones/deadlock/memoria_deadlock.config memoria.config	
	cd ../cpu
	cp ../IPs/configuraciones/deadlock/cpu_deadlock.config cpu.config
	cd ../filesystem
	cp ../IPs/configuraciones/deadlock/filesystem_deadlock.config filesystem.config

}

Config_Memoria(){
	cd kernel
	cp ../IPs/configuraciones/memoria/kernel_memoria.config kernel.config	
	cd ../memoria
	while true; do
		echo "1)BEST \n 2)WORST\n 3)FIRST\n "
		read choice;
			case $choice in
				1) cp ../IPs/configuraciones/memoria/memoria_memoria_BEST.config memoria.config;;
				2) cp ../IPs/configuraciones/memoria/memoria_memoria_WORST.config memoria.config;;
				3) cp ../IPs/configuraciones/memoria/memoria_memoria_FIRST.config memoria.config;;
			esac
		break
	done
	cd ../cpu
	cp ../IPs/configuraciones/memoria/cpu_memoria.config cpu.config
	cd ../filesystem
	cp ../IPs/configuraciones/memoria/filesystem_memoria.config filesystem.config

}

Config_File_System(){
	cd kernel
	cp ../IPs/configuraciones/filesystem/kernel_fs.config kernel.config	
	cd ../memoria
	cp ../IPs/configuraciones/filesystem/memoria_fs.config memoria.config	
	cd ../cpu
	cp ../IPs/configuraciones/filesystem/cpu_fs.config cpu.config
	cd ../filesystem
	cp ../IPs/configuraciones/filesystem/filesystem_fs.config filesystem.config

}

Config_Errores(){
	cd kernel
	cp ../IPs/configuraciones/kernel_errores.config kernel.config	
	cd ../memoria
	cp ../IPs/configuraciones/memoria_errores.config memoria.config	
	cd ../cpu
	cp ../IPs/configuraciones/cpu_errores.config cpu.config
	cd ../filesystem
	cp ../IPs/configuraciones/filesystem_errores.config filesystem.config

}

##------------------------------------ENTREGA FINAL PRUEBAS-----------------------------------------------------------------------
#Pruebas Finales
RealizarPruebasFinales(){
	cd ~/tuki-pruebas
	path_tuki=$(pwd)
	
	cd ~/tp-2023-1c-maxikiosco/
	make clean
	make all
	cd consola
	clear

	while true; do
		echo "PRUEBAS FINALES: ¿Que Prueba desea realizar? 1: Prueba Base; 2: Prueba Deadlock; 3: Prueba Memoria; 4: Prueba FileSystem; 5: Prueba Errores; 0: Salir "
		read choice;
			case $choice in
				1) PruebaBase;;
				2) PruebaDeadlock;;
				3) PruebaMemoria;;
				4) PruebaFileSystem;;
				5) PruebaErrores;;
				0) break;;
			esac
	done
}


PruebaBase(){
	echo "\n Ejecutando pruebas BASE:\n"
	
	gnome-terminal --title "Consola #A (BASE_1)" -- bash -c "./consola.exe consola.config ${path_tuki}/BASE_1; exec bash"
	gnome-terminal --title "Consola #B (BASE_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/BASE_2; exec bash"
	gnome-terminal --title "Consola #C (BASE_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/BASE_2; exec bash"
	
	echo "\n Pruebas BASE ejecutadas\n"
}

PruebaDeadlock(){
	echo "\n Ejecutando pruebas de DEADLOCK:\n"

	gnome-terminal --title "Consola #A (DEADLOCK_1)" -- bash -c "./consola.exe consola.config ${path_tuki}/DEADLOCK_1; exec bash"
	gnome-terminal --title "Consola #B (DEADLOCK_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/DEADLOCK_2; exec bash"
	gnome-terminal --title "Consola #C (DEADLOCK_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/DEADLOCK_3; exec bash"

	echo "\n ¿Continuar con DEADLOCK_4? (ok?)\n"
	read choice;

	gnome-terminal --title "Consola #D (DEADLOCK_4)" -- bash -c "./consola.exe consola.config ${path_tuki}/DEADLOCK_4; exec bash"

	echo "\n Pruebas de DEADLOCK ejecutadas. Recordar correr Deadlock_4 a mano\n"
}

PruebaMemoria(){
	echo "\n Ejecutando pruebas MEMORIA:\n"

	gnome-terminal --title "Consola #A (MEMORIA_1)" -- bash -c "./consola.exe consola.config ${path_tuki}/MEMORIA_1; exec bash"
	gnome-terminal --title "Consola #B (MEMORIA_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/MEMORIA_2; exec bash"
	gnome-terminal --title "Consola #C (MEMORIA_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/MEMORIA_3; exec bash"

	echo "\n Pruebas MEMORIA ejecutadas\n"
}

PruebaFileSystem(){
	echo "\n Ejecutando pruebas FILESYSTEM:\n"
	gnome-terminal --title "Consola #A (FS_1)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_1; exec bash"
	gnome-terminal --title "Consola #B (FS_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_2; exec bash"
	
	echo "\n ¿Cerraste los servidores? (ok?)\n"
	read choice;
	echo "\n ¿Continuar con FS_3? (ok?)\n"
	read choice;

	gnome-terminal --title "Consola #C (FS_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_3; exec bash"
	gnome-terminal --title "Consola #D (FS_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_3; exec bash"
	gnome-terminal --title "Consola #E (FS_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_3; exec bash"
	gnome-terminal --title "Consola #F (FS_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/FS_3; exec bash"

	echo "\n Pruebas FILESYSTEM ejecutadas\n"
}

PruebaErrores(){
	echo "\n Ejecutando pruebas ERRORES:\n"

	gnome-terminal --title "Consola #A (ERROR_1)" -- bash -c "./consola.exe consola.config ${path_tuki}/ERROR_1; exec bash"
	
	echo "\n ¿Continuar con el ERROR_2? (ok?)\n"
	read choice;

	gnome-terminal --title "Consola #B (ERROR_2)" -- bash -c "./consola.exe consola.config ${path_tuki}/ERROR_2; exec bash"
	
	echo "\n ¿Continuar con el ERROR_3? (ok?)\n"
	read choice;

	gnome-terminal --title "Consola #C (ERROR_3)" -- bash -c "./consola.exe consola.config ${path_tuki}/ERROR_3; exec bash"
	
	echo "\n ¿Continuar con el ERROR_4? (ok?)\n"
	read choice;

	gnome-terminal --title "Consola #D (ERROR_4)" -- bash -c "./consola.exe consola.config ${path_tuki}/ERROR_4; exec bash"
	
	echo "\n Pruebas ERRORES ejecutadas\n"
}
##------------------------------------Conseguir la IP-----------------------------------------------------------------------
DameLaIP(){
	echo "\n-----------------------\n"
	ip addr show | grep -oP 'inet \K(?!127\.0\.0\.1)[\d.]+'
	echo "\n-----------------------\n"
}

##-----------------------------------------Pushar_IP------------------------------------

Pushar_IP(){
	clear
	echo "\n Pusheando a nube\n"
	cd ~/tp-2023-1c-maxikiosco/
	
	git add .
	git commit -m "Pusheo automatico para la actualizacion de IPs"
	git push
	echo "\n Se pusheo a nube.\n"
}


##------------------------------------Execution-----------------------------------------------------------------------
#Mensaje de bienvenida (general) + limpiado de pantalla
clear
echo "Hola, y bienvenido al tp de sistemas operativos MAXIKIOSCO"

#Chequeo de parametros

#Parametro Help (Comentado hasta futura actu)
if [ "$1" = "help" ]; then
	echo "Utilizaste el parametro help..."
	help
	exit
fi

#Ejecución del programa (SIN PARAMETROS)
while true; do
	echo "¿Que desea hacer?\n 1: PREPARAR PARA ENTREGA;\n 2: Cambiar la config;\n 3: REALIZAR PRUEBAS FINALES;\n 4: Conseguir la IP;\n 5: Pushear IP;\n 0: Salir "
	read choice;
		case $choice in
			1) EntregaFinal;;
			2) CambiarConfig;;
			3) RealizarPruebasFinales;;
			4) DameLaIP;;
			5) Pushar_IP;;
			0) exit;;
		esac
done
