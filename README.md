# Макаревич Мария, БПИ 228
Индивидуальное домашнее задание 2 по дисциплине "Операционные системы".
**Работа выполнена на 8 баллов.**
## Вариант 3
Задача о Винни-Пухе – 2 или неправильные пчелы. N пчел живет в улье. Каждая пчела может собирать мед или сторожить улей (N > 3). Пчела не покинет улей, если кроме нее в нем нет других пчел. Каждая пчела приносит за раз одну порцию меда. Всего в улей может войти тридцать порций меда. Вини-Пух спит пока меда в улье меньше половины, но как только его становится достаточно, он просыпается и пытается достать весь мед из улья. Если в улье находится меньше трех пчел, Вини-Пух забирает мед, убегает, съедает мед и снова засыпает. Если пчел три или больше, то они кусают Винни-Пуха, он убегает, лечит укус некоторое время, а затем снова бежит за медом. Создать приложение, моделирующее поведение пчел и медведя. Осуществить балансировку, обеспечивающую циклическое освобождение улья от меда. Медведя и каждую из пчел представить отдельными процессами.
## 4-5 баллов
### Алгоритм решения
В программе каждую пчелу, а также Винни Пуха, олицетворяет отдельный процесс-ребенок. Все они запускаются в одном файле с помощью команды fork(). Количество пчел в улье задается пользователем через командуую строку. **Тестировать программу удобнее всего при количестве пчел, равном 4.** При неверном формате или содержании аргументов пользователю выводится сообщение об ошибке.

В программе используется объект разделяемой памяти honey_portions - количество порций меда в улье, а также два **именованных** семафора: bees, отвечающий за количество пчел, которые могут покинуть улей, и hive_access, отвечающий за доступ к улью (чтобы Винни Пух и пчелы не могли одновременно проводить операции с ульем и, соответственно, honey_portions). Отметим, что из семафора bees можно также понять количество пчел, находящихся в улье, - оно равно значению семафора, увеличенному на 1, - что нужно для алгоритма кражи меда.

Пчелы на протяжении всей работы программы вылетают из улья, если у них есть такая возможность (т.е. bees != 0), через некоторое время (реализуемое вызовом команды sleep()) возвращаются в улей с медом, пополняют его запасы в улье, то есть увеличивают honey_portions, если это возможно, и на какое-то время остаются в улье, после чего процесс повторяется снова. Винни Пух наблюдает за ульем и, когда в улье накапливается достаточное количество меда (то есть honey_portions становится больше или равно 15), пытается украсть его. Далее, в зависимости от количества пчел в улье, получаемом из значения семафора bees, Винни либо опустошает улей, приравнивая honey_portions к 0, либо, покусанный пчелами, уходит лечиться, после чего процесс повторяется снова.

В программе предусмотрено ее корректное завершение с удалением разделяемой памяти и семафоров по прерыванию с клавиатуры, то есть при команде SIGINT.

## Пример результатов работы программы
![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/c59021ea-fd92-4a3a-bd5d-bef783b17e62)

\*долгий и скучный процесс заполнения улья медом*

![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/fbb5f4b7-fd8c-4b38-ba03-c8091cb1b228)
## 6-7 баллов
### Алгоритм решения
Логика работы программы остается прежней, однако семафоры bees и hive_access заменяются на **неименованные**. Они хранятся в разделяемой памяти вместе с honey_portions с помощью структуры HiveData.
## Пример результатов работы программы
![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/b4fc0d09-eb5b-4018-b945-49d2b556f338)

\*долгий и скучный процесс заполнения улья медом*

![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/6fa67986-b50e-4504-b490-28179bce4a2c)
## 8 баллов
### Алгоритм решения
Данная программа состоит из 2 файлов: bear.c и bee.c. Для корректной работы приложения необходимо запустить программу bear.c с аргументом N, а затем запустить N программ bee.c параллельно. Впрочем, можно запускать и меньше программ bee.c, тогда часть пчел будет просто сидеть в улье, а не летать за медом. Приложение завершает свою работу по сигналу SIGINT, полученному в программе Винни Пуха. Таким образом, программу Винни Пуха следует запускать первой и прекращать последней для корректной работы приложения.

В данной программе используются три семафора стандарта UNIX SYSTEM V, работа с которыми ведется через их id: bees_id, access_id, portions_id. Данные семафоры являются заменой bees, hive_access и honey_portions из предыдущих программ соответственно. Таким образом, в данной программе информация о количестве порций меда в улье хранится не как число в разделяемой памяти, а как семафор. В остальном логика программы остается прежней.
## Пример результатов работы программы
bear.c:

![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/3b51412e-8aea-4681-9bec-0bc9c0322336)

bee.c:

![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/d59e38f5-ffba-438f-ab60-67ca2d213f4e)

## Пчелки
С Вашей помощью они справились с медведем, они довольны!
![image](https://github.com/programmer-owl/OS-IHW2/assets/131264233/82ce6039-5309-4e7c-9f84-360767ed4e21)

