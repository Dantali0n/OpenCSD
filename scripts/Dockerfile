FROM archlinux:base-devel

# Configure mirrors including multilib
#  - echo "Server = https://mirrors.daan.vodka/archlinux/$repo/os/$arch" | sudo tee /etc/pacman.d/mirrormulti
RUN echo "[multilib]" | sudo tee -a /etc/pacman.conf; \
    echo "Include = /etc/pacman.d/mirrorlist" | sudo tee -a /etc/pacman.conf

# Install required dependencies through pacman
RUN pacman -Syy && \
    sudo pacman -Syu --noconfirm && \
    sudo pacman -Sy --noconfirm cmake clang gcc llvm yasm ninja \
    cunit pixman python-pip git libaio numactl lib32-glibc gcovr valgrind \
    doxygen texlive-bin texlive-science texlive-publishers texlive-latexextra \
    texlive-humanities

# Install meson and pyelftools through pip
RUN pip3 install meson==0.59 pyelftools

# Create builduser and setup sudoers
RUN useradd builduser -m; \
    passwd -d builduser; \
    printf 'builduser ALL=(ALL) ALL\n' | tee -a /etc/sudoers

# Build and install lcov from aur using the builduser
RUN sudo -u builduser bash -c 'cd ~ && git clone https://aur.archlinux.org/lcov.git && cd lcov && makepkg -si --noconfirm'

## cleanup files from setup
RUN sudo pacman -Sc --noconfirm; rm -rf /var/cache/pacman/pkg/ /tmp/* /var/tmp/*
