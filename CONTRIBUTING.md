<div align="center">

# Contributing to Salmon
</div>

<div align="center">

### Should I contribute
</div>

Yes, you should! If you are reading this you most likely are already considering it, so this is here to help nudge you in the right direction.  
Contributing allows passionate users to add features that they want, fix bugs, add documantation, and optimize compiletime/runtime, improving overall user experiance.

---
<div align="center">

### How to Contribute
</div>

Thank you for considering contributing to this humble programming lanuguage, it truly means the most.
#### How to get this repo on your machine
1. fork the repo with all branches to your github account.

2. clone the repo to your machine
```bash
git clone "your_forked_repo"
```
> replace  "your_forked_repo" with the git link to the fork of the Salmon repo
#### What to do to contibute
1. switch to the `develpoment` branch with
```bash
git checkout development
```
2. make your changes, please stick to the style guide [styleguide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/) and use `///` comments for functions, describing what it does and how it does what it does
3. test as much as possible

4. add it
```bash
git add .
```
or
```language
git add "your file(s)"
```
> replace "your file(s)" with the path to the file(s) changed
5. commit the changes
```bash
git commit -m "'done' ['something']"
```
> replace 'done' with either added, implemented, or something else  
replace 'something' with a feature
6. push changes to forked repo
```bash
git push -u origin development
```
7. if you are fixing a bug, create an issue report in the Salmon repo

8. create a pull request with a descriptave title and a detailed description, if you are fixing a bug, mark it as resolveing an issue
