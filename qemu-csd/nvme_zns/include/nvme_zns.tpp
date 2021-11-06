template<class nvme_zns_backend>
NvmeZns<nvme_zns_backend>::NvmeZns(nvme_zns_backend* backend) {
    this->backend = backend;
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::read(
    uint64_t zone, uint64_t sector, size_t offset, void* buffer, size_t size)
{
    return this->backend->read(zone, sector, offset, buffer, size);
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::append(
    uint64_t zone, uint64_t& sector, size_t offset, void* buffer, size_t size)
{
    return this->backend->append(zone, sector, offset, buffer, size);
}

template<class nvme_zns_backend>
int NvmeZns<nvme_zns_backend>::reset(uint64_t zone) {
    return this->backend->reset(zone);
}
